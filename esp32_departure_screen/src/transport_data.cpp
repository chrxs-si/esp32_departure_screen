#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "display.h"
#include "transport_data.h"

Departure departures[MAX_DEPARTURES] = {};

long isoToRelativeMinutes(const String& isoTime) {
  struct tm tm = {};

  // "2026-01-27T18:03:00+01:00" → ohne Zeitzone parsen
  strptime(isoTime.substring(0, 19).c_str(),
            "%Y-%m-%dT%H:%M:%S",
            &tm);

  time_t target = mktime(&tm);

  time_t now;
  time(&now);

  return (target - now) / 60;
}

String getDeparturesJson(String stopID) {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Nicht mit WLAN verbunden!");
    String json = "{\"message\":\"Wifi error\"}";
    return json;
  }

  String apiURL = "https://v6.vbb.transport.rest/stops/" + stopID + "/departures?results=" + MAX_DEPARTURES + "&duration=30";
  Serial.println("API URL: " + apiURL);
  HTTPClient http;
  http.begin(apiURL);
  int httpCode = http.GET();

  String json = "";
  if(httpCode > 0) {
    json = http.getString();
    Serial.println("Successful API request.");
  } else {
    json = "{\"message\":\"API error\", \"code\":" + String(httpCode) + "}";
    Serial.println("Fehler bei der API-Anfrage, HTTP Code: " + String(httpCode));
  }

  http.end();
  return json;
}

int parseDepartures(String json, String lineFilter1, String lineFilter2, Departure* result, int maxResults) {
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

        if (!lineName[0] || !dest[0] || !when[0]) continue;

        bool useLineFilter1 = lineFilter1 && lineFilter1 != "";
        bool useLineFilter2 = lineFilter2 && lineFilter2 != "";

        if ((useLineFilter1 || useLineFilter2) && String(lineName) != lineFilter1 && String(lineName) != lineFilter2) continue;
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
  int numDepartures = parseDepartures(json, selectedLine, selectedLine2, departures, MAX_DEPARTURES);
  int maxShownDepartures = 3;
  if (!showWeatherTime) {
    maxShownDepartures = 4;
  }
  int coloums = min(numDepartures, maxShownDepartures);

  if (coloums == 0) {

    if (retryCount < 3) {
      retryCount++;
      return;
    }

    if (!showWeatherTime) {
      drawStaticText("", 0, PANEL_RES_X, ROW_1, ALIGN_CENTER, BLACK, 1);
    }
    drawStaticText("Keine", 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, DEPARTURE_COLOR, 1);
    drawStaticText("Abfahrten", 0, PANEL_RES_X, ROW_3, ALIGN_CENTER, DEPARTURE_COLOR, 1);
    drawStaticText("", 0, PANEL_RES_X, ROW_4, ALIGN_CENTER, BLACK, 1);
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

    // Länge begrenzen
    if (dest.length() > MAX_DEST_LEN) {
      dest = dest.substring(0, MAX_DEST_LEN);
    }

    String lineString = "";
    if (showLine) {
      lineString += String(departures[i].line) + " ";
    } 

    if (showWeatherTime) {
      drawStaticText(lineString + dest, 0, PANEL_RES_X - 12, getVerticalPosForRow(i+1), ALIGN_LEFT, DEPARTURE_COLOR, 1);
      drawStaticText(String(departures[i].minutes), PANEL_RES_X - 12, PANEL_RES_X, getVerticalPosForRow(i+1), ALIGN_RIGHT, DEPARTURE_COLOR, 1);
    } else {
      drawStaticText(dest, 0, PANEL_RES_X - 12, getVerticalPosForRow(i), ALIGN_LEFT, DEPARTURE_COLOR, 1);
      drawStaticText(String(departures[i].minutes), PANEL_RES_X - 12, PANEL_RES_X, getVerticalPosForRow(i), ALIGN_RIGHT, DEPARTURE_COLOR, 1);
    }
  }
}