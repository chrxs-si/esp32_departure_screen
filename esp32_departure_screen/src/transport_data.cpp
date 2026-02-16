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

String getDeparturesJson(String stopID, String lineFilter, int maxColumns) {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Nicht mit WLAN verbunden!");
    String json = "{\"message\":\"Wifi error\"}";
    return json;
  }

  String apiURL = "https://v6.vbb.transport.rest/stops/" + stopID + "/departures?results=" + MAX_DEPARTURES + "&duration=30";
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

int parseDepartures(String json, const char* lineFilter, Departure* result, int maxResults) {
    DynamicJsonDocument doc(16384);  // größerer Speicher

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

        if (lineFilter && strcmp(lineName, lineFilter) != 0) continue;
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

void updateDepartures(int retrys) {
  Serial.print("update departures.");

  String json = getDeparturesJson(stop1, line1, maxColumns1);
  int numDepartures = parseDepartures(json, line1.c_str(), departures, MAX_DEPARTURES);
  int coloums = min(min(numDepartures, maxColumns1), 3);

  if (coloums == 0) {

    if (retrys < 2) {
      Serial.println("Keine Abfahrten gefunden, versuche es erneut...");
      delay(1000);
      updateDepartures(retrys + 1);
    } else {
      Serial.println("Nach mehreren Versuchen keine Abfahrten gefunden.");
      return;
    }

    drawStaticText("Keine", 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, DEPARTURE_COLOR, 1);
    drawStaticText("Abfahrten", 0, PANEL_RES_X, ROW_3, ALIGN_CENTER, DEPARTURE_COLOR, 1);
    drawStaticText("", 0, PANEL_RES_X, ROW_4, ALIGN_CENTER, DEPARTURE_COLOR, 1);
    return;
  }

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

    drawStaticText(dest, 0, PANEL_RES_X - 12, getVerticalPosForRow(i+1), ALIGN_LEFT, DEPARTURE_COLOR, 1);
    drawStaticText(String(departures[i].minutes), PANEL_RES_X - 12, PANEL_RES_X, getVerticalPosForRow(i+1), ALIGN_RIGHT, DEPARTURE_COLOR, 1);
  }
}