#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "display.h"
#include "transport_data.h"

Departure departures[MAX_DEPARTURES] = {};
String BVG_BASE_URL = "https://v6.bvg.transport.rest/stops/";
String VBB_BASE_URL = "https://v6.vbb.transport.rest/stops/";
bool useVBB = true;

long isoToRelativeMinutes(const String& isoTime) {
  struct tm tm = {0};
  int year, month, mday, hour, min, sec, tz_h, tz_m;
  char tz_sign;

  // Beispiel: 2026-01-27T18:03:00+01:00
  // Wir parsen alle Komponenten inklusive Zeitzonen-Offset
  if (sscanf(isoTime.c_str(), "%d-%d-%dT%d:%d:%d%c%d:%d", 
             &year, &month, &mday, &hour, &min, &sec, 
             &tz_sign, &tz_h, &tz_m) < 7) {
    return 0;
  }

  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = mday;
  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_sec = sec;
  tm.tm_isdst = -1; // Wichtig: mktime soll Sommerzeit selbst prüfen

  // mktime konvertiert struct tm (Lokalzeit laut TZ-Variable) in Unix-Timestamp
  time_t target = mktime(&tm);
  
  time_t now;
  time(&now);

  // Falls mktime und die API-Zeit unterschiedliche Offsets haben (UTC vs. Local),
  // gleicht dies die Differenz aus.
  long diffSeconds = difftime(target, now);
  
  return diffSeconds / 60;
}

String getDeparturesJson(String stopID) {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Nicht mit WLAN verbunden!");
    String json = "{\"message\":\"Wifi error\"}";
    return json;
  }

  time_t now;
  time(&now);
  time_t targetTime = now + 12 + (offsetMin * 60); // 12 Sekunden addieren, damit keine negativen Abfahrtswerte entstehen

  String apiURL; 
  if (useVBB) {
    apiURL = VBB_BASE_URL + stopID + "/departures?results=" + String(MAX_DEPARTURES) + "&duration=30&remarks=false&when=" + String(targetTime);
  } else {
    apiURL = BVG_BASE_URL + stopID + "/departures?results=" + String(MAX_DEPARTURES) + "&duration=30&remarks=false&when=" + String(targetTime);
  }

  Serial.println("API URL: " + apiURL);
  HTTPClient http;
  http.begin(apiURL);
  http.setTimeout(10000);
  int httpCode = http.GET();

  String json = "";
  if(httpCode > 0) {
    json = http.getString();
    Serial.println("Successful API request.");
    
  } else {
    json = "{\"message\":\"API error\", \"code\":" + String(httpCode) + "}";
    Serial.println("Fehler bei der API-Anfrage, HTTP Code: " + String(httpCode));

    // andere URL nutzen
    useVBB = !useVBB;
  }

  http.end();
  return json;
}

int parseDepartures(String json, String lineFilter1, String lineFilter2, String lineFilter3, String lineFilter4, Departure* result, int maxResults) {
    DynamicJsonDocument doc(32768);  // größerer Speicher

    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.print("JSON Fehler: ");
        Serial.println(err.c_str());
        return -1;
    }

    JsonArray departures = doc["departures"].as<JsonArray>();
    if (departures.isNull()) return 0;

    Serial.println("departures:");
    serializeJson(departures, Serial);
    Serial.println();

    int count = 0;
    for (JsonObject dep : departures) {
        const char* lineName = dep["line"]["name"] | "?";
        const char* dest     = dep["direction"] | "?";
        const char* when     = dep["when"] | "?";
        bool cancelled = dep["cancelled"] | false;

        if (!lineName[0] || !dest[0] || !when[0] || cancelled) continue;

        bool useLineFilter1 = lineFilter1 && lineFilter1 != "";
        bool useLineFilter2 = lineFilter2 && lineFilter2 != "";
        bool useLineFilter3 = lineFilter3 && lineFilter3 != "";
        bool useLineFilter4 = lineFilter4 && lineFilter4 != "";

        if ((useLineFilter1 || useLineFilter2 || useLineFilter3 || useLineFilter4) && String(lineName) != lineFilter1 && String(lineName) != lineFilter2 && String(lineName) != lineFilter3 && String(lineName) != lineFilter4) continue;
        if (count >= maxResults) break;

        strncpy(result[count].line, lineName, MAX_LINE_LEN - 1);

        result[count].destination = dest;
        result[count].minutes = isoToRelativeMinutes(when);
        result[count].delay = dep["delay"].isNull() ? 0 : dep["delay"].as<int>() / 60;

        count++;
    }

    return count;
}

String cutString(const String& str, int maxLength) {
  if (str.length() <= maxLength) return str;
  return str.substring(0, maxLength);
}

int retryCount = 0;

void updateDepartures() {
  Serial.print("update departures.");

  String json = getDeparturesJson(selectedStopID);
  int numDepartures = parseDepartures(json, selectedLine, selectedLine2, selectedLine3, selectedLine4, departures, MAX_DEPARTURES);
  int maxShownDepartures = 3;
  if (!showWeatherTime) {
    maxShownDepartures = 4;
  }
  int coloums = min(numDepartures, maxShownDepartures);

  if (coloums == 0) {

    if (retryCount < 5) {
      retryCount++;
        Serial.println("Keine Abfahrten gefunden. Retry: " + String(retryCount));
      return;
    }

    if (!showWeatherTime) {
      drawStaticText("", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, BLACK, 1);
    }
    drawStaticText("Keine", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, DEPARTURE_COLOR, 1);
    drawStaticText("Abfahrten", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, DEPARTURE_COLOR, 1);
    drawStaticText("", 0, PANEL_RES_X * PANEL_CHAIN, ROW_4, ALIGN_CENTER, BLACK, 1);
    return;
  }

  retryCount = 0;

  for(int i=0; i<coloums; i++) {
    String dest = String(departures[i].destination);

    // Anfang (wie z.B. "U", "S+U" abschneiden)
    int pos = dest.indexOf(' ');
    if (pos != -1 && pos < 4) {
        dest = dest.substring(pos + 1);
    }
    
    // Liste mit Endungen die Abgeschnitten werden
    const char* endings[] = {
        "(Berlin)",
        "(bei Berlin)",
        "⟲",
        "⟳",
        "S41",
        "S42"
    };
    const int endingCount = sizeof(endings) / sizeof(endings[0]);

    // Endungen prüfen und entfernen
    for (int i = 0; i < endingCount; i++) {
        String endStr = String(endings[i]);

        if (dest.endsWith(endStr)) {
            dest = dest.substring(0, dest.length() - endStr.length());
            break;
        }
    }

    // Länge begrenzen
    int dest_len = MAX_DEST_LEN_1_PANEL;
    if (PANEL_CHAIN == 2) dest_len = MAX_DEST_LEN_2_PANEL;
    if (showLine) {dest_len -= MAX_LINE_LEN;}
    if (dest.length() > dest_len) {
      dest = dest.substring(0, dest_len);
    }

    // constum Ziele setzen
    if (dest == replaceFrom1) {
      dest = replaceTo1;
    }
    if (dest == replaceFrom2) {
      dest = replaceTo2;
    }
    if (dest == replaceFrom3) {
      dest = replaceTo3;
    }
    if (dest == replaceFrom4) {
      dest = replaceTo4;
    }

    String lineString = "";
    if (showLine) {
      lineString += String(departures[i].line) + " ";
    } 

    if (showWeatherTime) {
      drawStaticText("A" + String(departures[i].minutes) + "A", PANEL_RES_X * PANEL_CHAIN - 12, PANEL_RES_X * PANEL_CHAIN, getVerticalPosForRow(i+1), ALIGN_RIGHT, DEPARTURE_COLOR, 1);
      drawStaticText(lineString + dest, 0, PANEL_RES_X * PANEL_CHAIN - 12, getVerticalPosForRow(i+1), ALIGN_LEFT, DEPARTURE_COLOR, 1);
    } else {
      drawStaticText("A" + String(departures[i].minutes) + "A", PANEL_RES_X * PANEL_CHAIN - 12, PANEL_RES_X * PANEL_CHAIN, getVerticalPosForRow(i), ALIGN_RIGHT, DEPARTURE_COLOR, 1);
      drawStaticText(lineString + dest, 0, PANEL_RES_X * PANEL_CHAIN - 12, getVerticalPosForRow(i), ALIGN_LEFT, DEPARTURE_COLOR, 1);
    }
  }
}