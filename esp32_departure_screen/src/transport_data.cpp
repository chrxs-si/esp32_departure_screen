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

  String json = "{\"message\":\"API error\"}";
  if(httpCode > 0) {
    json = http.getString();
    Serial.println("Successful API request.");
  } else {
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
        return 0;
    }

    JsonArray departures = doc["departures"].as<JsonArray>();
    if (departures.isNull()) return 0;

    Serial.println("departures:");
    serializeJson(departures, Serial);
    Serial.println();

    int count = 0;
    for (JsonObject dep : departures) {
        const char* lineName = dep["line"]["name"] | "?";
        const char* dest     = dep["destination"]["name"] | "?";
        const char* when     = dep["when"] | "?";

        if (!lineName[0] || !dest[0] || !when[0]) continue;

        if (lineFilter && strcmp(lineName, lineFilter) != 0) continue;
        if (count >= maxResults) break;

        strncpy(result[count].line, lineName, MAX_LINE_LEN - 1);
        strncpy(result[count].destination, dest, MAX_DEST_LEN - 1);

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

void updateDepartures() {
  Serial.println("Aktualisiere Abfahrten...");
  Serial.println("get Departures...");
  String json = getDeparturesJson(stop1, line1, maxColumns1);
  Serial.println("parse Departures...");
  int numDepartures = parseDepartures(json, line1.c_str(), departures, MAX_DEPARTURES);
  Serial.println("numDepartures: " + numDepartures);

  Serial.println("Update Display...");
  int coloums = min(min(numDepartures, maxColumns1), 2);

  for(int i=0; i<coloums; i++) {
    String dest = cutString(String(departures[i].destination), MAX_DEST_LEN);

    drawStaticText(dest, 0, PANEL_RES_X, getVerticalPosForRow(i+1), ALIGN_LEFT, DEPARTURE_COLOR, 1);
    drawStaticText(String(departures[i].minutes), 0, PANEL_RES_X, getVerticalPosForRow(i+1), ALIGN_RIGHT, DEPARTURE_COLOR, 1);
  }
}