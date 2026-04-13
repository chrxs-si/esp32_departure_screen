#include "wifi_setup.h"
#include "display.h"
#include "main.h"
#include "weather_icons.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

Preferences prefs;

#define DNS_PORT 53

WebServer server(80);
DNSServer dnsServer;

String espSSID = "SETUP-" + String(random(100, 1000));
String espPassword = String(random(10000000, 100000000));

// Globale Variablen
bool inConfigMode = true; // True = WLAN bleibt offen / Setup erzwingen
bool setupComplete = false;

String current_version = "1.0.0";

String selectedSSID = "";
String selectedPassword = "";

String selectedStopName = "";
String selectedStopID = "";
String selectedLine = "";
String selectedLine2 = "";
String selectedLine3 = "";
String selectedLine4 = "";

int offsetMin = 0; 

String displayColorName = "ORANGE";
String timeColorName = "BLUE";

float latitudeStop = 52.5170365;
float longitudeStop = 13.3888599;

bool showLine = true;
bool showWeatherTime = false;

String replaceFrom1 = "";
String replaceTo1   = "";
String replaceFrom2 = "";
String replaceTo2   = "";
String replaceFrom3 = "";
String replaceTo3   = "";
String replaceFrom4 = "";
String replaceTo4   = "";

String wifiOptionsHTML = "";

//extern
bool ads = false;
int adsInterval = 60;
bool discoMode = false;
int discoTime = 30;


/* ========================================================= */
/* EINSTELLUNGEN LADEN & SPEICHERN
/* ========================================================= */

bool loadSettings() {
  prefs.begin("config", true);

  inConfigMode = prefs.getBool("configMode", true);

  current_version = prefs.getString("current_version", current_version);

  espSSID = prefs.getString("esp_ssid", espSSID);
  espPassword = prefs.getString("esp_pass", espPassword);

  selectedSSID = prefs.getString("ssid", "");
  selectedPassword = prefs.getString("pass", "");

  selectedStopName = prefs.getString("stopName", "");
  selectedStopID = prefs.getString("stopID", "");

  selectedLine = prefs.getString("line1", "");
  selectedLine2 = prefs.getString("line2", "");
  selectedLine3 = prefs.getString("line3", "");
  selectedLine4 = prefs.getString("line4", "");

  showLine = prefs.getBool("showLine", false);
  showWeatherTime = prefs.getBool("weather", true);

  offsetMin = prefs.putUInt("offsetMin", 0);

  displayColorName = prefs.getString("displayColorName", "ORANGE");
  timeColorName = prefs.getString("timeColorName", "BLUE");

  replaceFrom1 = prefs.getString("replaceFrom1", "");
  replaceTo1 = prefs.getString("replaceTo1", "");
  replaceFrom2 = prefs.getString("replaceFrom2", "");
  replaceTo2 = prefs.getString("replaceTo2", "");
  replaceFrom3 = prefs.getString("replaceFrom3", "");
  replaceTo3 = prefs.getString("replaceTo3", "");
  replaceFrom4 = prefs.getString("replaceFrom4", "");
  replaceTo4 = prefs.getString("replaceTo4", "");

  latitudeStop = prefs.getFloat("lat", 52.498882);
  longitudeStop = prefs.getFloat("lon", 13.371630);

  prefs.end();

  // Überprüfen, ob grundlegende WLAN- und Stationsdaten vorhanden sind
  if (selectedSSID == "" || selectedPassword == "" || selectedStopID == "") {
    return false;
  }
  return true;
}

void saveSettings() {
  prefs.begin("config", false);

  prefs.putString("current_version", current_version);

  prefs.putString("esp_ssid", espSSID);
  prefs.putString("esp_pass", espPassword);

  prefs.putString("ssid", selectedSSID);
  prefs.putString("pass", selectedPassword);

  prefs.putString("stopName", selectedStopName);
  prefs.putString("stopID", selectedStopID);

  prefs.putString("line1", selectedLine);
  prefs.putString("line2", selectedLine2);
  prefs.putString("line3", selectedLine3);

  prefs.putBool("showLine", showLine);
  prefs.putBool("weather", showWeatherTime);

  prefs.putUInt("offsetMin", offsetMin);

  prefs.putString("displayColorName", displayColorName);
  prefs.putString("timeColorName", timeColorName);

  prefs.putString("replaceFrom1", replaceFrom1);
  prefs.putString("replaceTo1", replaceTo1);
  prefs.putString("replaceFrom2", replaceFrom2);
  prefs.putString("replaceTo2", replaceTo2);
  prefs.putString("replaceFrom3", replaceFrom3);
  prefs.putString("replaceTo3", replaceTo3);
  prefs.putString("replaceFrom4", replaceFrom4);
  prefs.putString("replaceTo4", replaceTo4);

  prefs.putFloat("lat", latitudeStop);
  prefs.putFloat("lon", longitudeStop);

  prefs.putBool("configMode", inConfigMode);

  prefs.end();
}

/* ========================================================= */
/* HILFSFUNKTIONEN NETZWERK
/* ========================================================= */

void startAP() {
  display->clearScreen();
  drawStaticText("starte", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(espSSID.c_str(), espPassword.c_str());
  delay(500);
}

void scanWIFIOptions() {
  wifiOptionsHTML = "";
  
  display->clearScreen();
  drawStaticText("scanne", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  int n = WiFi.scanNetworks(false, true);
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    wifiOptionsHTML += "<option value='" + ssid + "'>" + ssid + "</option>";
  }
  WiFi.scanDelete();
}

bool connectStoredWifi() {
  if (selectedSSID == "" || selectedPassword == "") return false;

  WiFi.mode(WIFI_AP_STA); // AP soll aktiv bleiben, auch wenn Station verbunden ist
  WiFi.begin(selectedSSID.c_str(), selectedPassword.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
    delay(200);
  }

  return WiFi.status() == WL_CONNECTED;
}

void connectToWifi(String ssid, String password) {
  WiFi.mode(WIFI_AP_STA);  // AP bleibt aktiv
  WiFi.begin(ssid.c_str(), password.c_str());
}

/* ========================================================= */
/* API HILFSFUNKTIONEN
/* ========================================================= */

String extractStationID(const String& rawId) {
    String result = rawId;

    // 1. Alles bis zum ersten ':' entfernen
    int colonPos = result.indexOf(':');
    if (colonPos != -1) result = result.substring(colonPos + 1);

    // 2. Alles bis zum ersten ':' des neuen Strings entfernen
    colonPos = result.indexOf(':');
    if (colonPos != -1) result = result.substring(colonPos + 1);

    // 3. Solange ':' vorkommt, alles ab dem letzten ':' entfernen
    colonPos = result.lastIndexOf(':');
    while (colonPos != -1) {
        result = result.substring(0, colonPos);
        colonPos = result.lastIndexOf(':');
    }

    return result;
}

/* ========================================================= */
/* Einstellungen HILFSFUNKTIONEN
/* ========================================================= */

uint16_t getDisplayColorFromString(String colorName) {
  if (colorName == "BLACK") return BLACK;
  if (colorName == "WHITE") return WHITE;
  if (colorName == "RED") return RED;
  if (colorName == "GREEN") return GREEN;
  if (colorName == "BLUE") return BLUE;
  if (colorName == "YELLOW") return YELLOW;
  if (colorName == "CYAN") return CYAN;
  if (colorName == "MAGENTA") return MAGENTA;
  if (colorName == "PURPLE") return PURPLE;
  if (colorName == "PINK") return PINK;
  if (colorName == "BROWN") return BROWN;
  if (colorName == "GRAY") return GRAY;
  if (colorName == "LIGHTGRAY") return LIGHTGRAY;
  if (colorName == "DARKGREEN") return DARKGREEN;
  if (colorName == "LIGHTBLUE") return LIGHTBLUE;
  
  return ORANGE; // Fallback, falls etwas schiefgeht
}

/* ========================================================= */
/* SEITEN & HANDLER
/* ========================================================= */

// --- SEITE 1: WLAN SETUP ---
void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>WLAN Setup</title>
  </head>
  <body>
    <h2>WLAN Konfiguration</h2>
    <p>Hallo! Schön das du es hierher geschafft hast. Um die Anzeige zu benutzen ist natürlich eine Internetverbindung nötig. Wähle also bitte unten dein WLAN aus und gebe das Passwort ein. Sollte dein WLAN nicht angezeigt werden, stecke das Display noch einmal vom Strom ab und wieder an. Mache das, wenn möglich, direkt neben dem WLAN-Router.</p>
    <p>Bitte dein WLAN auswählen und das Passwort eingeben:</p>
    <form action="/saveWifi" method="POST">
      <label>WLAN Netzwerk*:</label><br>
      <select name="ssid">
  )rawliteral";

  page += wifiOptionsHTML;

  page += R"rawliteral(
      </select><br><br>
      <label>WLAN Passwort*:</label><br>
      <input type="password" name="password"><br><br>
      <input type="submit" value="Speichern">
    </form>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

void handleSaveWifi() {
  if (server.hasArg("ssid")) selectedSSID = server.arg("ssid");
  if (server.hasArg("password")) selectedPassword = server.arg("password");

  display->clearScreen();
  drawStaticText("verbinde", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  connectToWifi(selectedSSID, selectedPassword);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 9000) {
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    display->clearScreen();
    drawStaticText("WLAN", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, RED, 1);
    drawStaticText("Fehler!", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, RED, 1);

    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");

    delay(4000);
    display->clearScreen();
    drawStaticText("Display-WLAN:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, WHITE, 1);
    drawStaticText(espSSID, 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("Passwort:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, WHITE, 1);
    drawStaticText(espPassword, 0, PANEL_RES_X * PANEL_CHAIN, ROW_4, ALIGN_CENTER, RED, 1);

    return;
  }

  display->clearScreen();
  drawStaticText("WLAN", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, GREEN, 1);
  drawStaticText("verbunden!", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, GREEN, 1);

  server.sendHeader("Location", "/configStop", true);
  server.send(302, "text/plain", "");

  delay(4000);
  display->clearScreen();
  drawStaticText("Station", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("eingeben!", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);
}

// --- SEITE 2: STATION SETUP ---
void handleStopConfig() {
  String page = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>Stationen Setup</title>
      <style>
        details { background: #f9f9f9; padding: 10px; border: 1px solid #ddd; border-radius: 5px; margin-bottom: 15px; }
        summary { font-weight: bold; cursor: pointer; margin-bottom: 10px; outline: none; }
      </style>
    </head>
    <body>
      <h2>Station Konfiguration</h2>
      <p>Super! WLAN haben wir schon einmal. Als nächstes musst du angeben, zu welcher Station du die Abfahrten angezeigt bekommen möchtest. Gebe dafür einen eindeutigen Teil des Stationsnamens ein (Bei "Warschauer Straße" reicht z.B. "Warschauer"). Bei Tippfehlern kann die Station leider nicht gefunden werden.</p>
      <p>Du kannst optional bis zu vier bestimmte Linien eingeben, welche Ausschließlich angezeigt werden sollen. Beispiele: Bus "M45", U-Bahn "U2", S-Bahn "S3", Tram "M10".</p>

      <form action="/saveStop" method="POST">
        <label>Station*:</label><br>
        <input type="text" name="stop" value=")rawliteral" + selectedStopName + R"rawliteral(" required><br><br>

        <label>1. Linie (optional): </label>
        <input type="text" name="line1" value=")rawliteral" + selectedLine + R"rawliteral("><br>

        <label>2. Linie (optional): </label>
        <input type="text" name="line2" value=")rawliteral" + selectedLine2 + R"rawliteral("><br>

        <label>3. Linie (optional): </label>
        <input type="text" name="line3" value=")rawliteral" + selectedLine3 + R"rawliteral("><br>

        <label>4. Linie (optional): </label>
        <input type="text" name="line4" value=")rawliteral" + selectedLine4 + R"rawliteral("><br><br>

        <input type="checkbox" name="weather" value="true" )rawliteral" + (showWeatherTime ? "checked" : "") + R"rawliteral(>
        Wetter und Uhrzeit anzeigen<br><br>

        <details>
          <summary>Erweitert</summary>
          
          <input type="checkbox" name="showLine" value="true" )rawliteral" + (showLine ? "checked" : "") + R"rawliteral(>
          Linie anzeigen<br><br>

          <label>Offset:</label><br>
          <small>Abfahrten unter dieser Zeit werden nicht mehr anzeigen.</small><br>
          <input type="number" name="offset" min="0" max="15" value=")rawliteral" + String(offsetMin) + R"rawliteral("> min<br><br>

          <label>Stationsnamen ersetzen</label><br>
          <small>Angezeigte Stationsnamen durch eigene Namen ersetzen.</small><br>

          <div style="display:flex; gap:10px; margin-bottom:6px;">
            <input style="flex:1;" type="text"
              name="replaceFrom1"
              value=")rawliteral" + replaceFrom1 + R"rawliteral("
              placeholder="Anzeigename">
            <input style="flex:1;" type="text" placeholder="Ersetzen mit"
              name="replaceTo1"
              value=")rawliteral" + replaceTo1 + R"rawliteral(">
          </div>

          <div style="display:flex; gap:10px; margin-bottom:6px;">
            <input style="flex:1;" type="text"
              name="replaceFrom2"
              value=")rawliteral" + replaceFrom2 + R"rawliteral("
              placeholder="Anzeigename">
            <input style="flex:1;" type="text"
              name="replaceTo2"
              value=")rawliteral" + replaceTo2 + R"rawliteral("
              placeholder="Ersetzen mit">
          </div>

          <div style="display:flex; gap:10px; margin-bottom:6px;">
            <input style="flex:1;" type="text"
              name="replaceFrom3"
              value=")rawliteral" + replaceFrom3 + R"rawliteral("
              placeholder="Anzeigename">
            <input style="flex:1;" type="text"
              name="replaceTo3"
              value=")rawliteral" + replaceTo3 + R"rawliteral("
              placeholder="Ersetzen mit">
          </div>

          <div style="display:flex; gap:10px; margin-bottom:6px;">
            <input style="flex:1;" type="text"
              name="replaceFrom4"
              value=")rawliteral" + replaceFrom4 + R"rawliteral("
              placeholder="Anzeigename">
            <input style="flex:1;" type="text"
              name="replaceTo4"
              value=")rawliteral" + replaceTo4 + R"rawliteral("
              placeholder="Ersetzen mit">
          </div>

          <br>
          <label>Anzeige Farbe:</label><br>
          <select name="displayColor" id="selDisplayColor">
            <option value="BLACK">Schwarz</option>
            <option value="WHITE">Weiß</option>
            <option value="RED">Rot</option>
            <option value="GREEN">Grün</option>
            <option value="BLUE">Blau</option>
            <option value="YELLOW">Gelb</option>
            <option value="CYAN">Türkis</option>
            <option value="MAGENTA">Magenta</option>
            <option value="ORANGE">Orange</option>
            <option value="PURPLE">Lila</option>
            <option value="PINK">Pink</option>
            <option value="BROWN">Braun</option>
            <option value="GRAY">Grau</option>
            <option value="LIGHTGRAY">Hellgrau</option>
            <option value="DARKGREEN">Dunkelgrün</option>
            <option value="LIGHTBLUE">Hellblau</option>
          </select><br><br>

          <label>Uhrzeit Farbe:</label><br>
          <select name="timeColor" id="selTimeColor">
            <option value="BLACK">Schwarz</option>
            <option value="WHITE">Weiß</option>
            <option value="RED">Rot</option>
            <option value="GREEN">Grün</option>
            <option value="BLUE">Blau</option>
            <option value="YELLOW">Gelb</option>
            <option value="CYAN">Türkis</option>
            <option value="MAGENTA">Magenta</option>
            <option value="ORANGE">Orange</option>
            <option value="PURPLE">Lila</option>
            <option value="PINK">Pink</option>
            <option value="BROWN">Braun</option>
            <option value="GRAY">Grau</option>
            <option value="LIGHTGRAY">Hellgrau</option>
            <option value="DARKGREEN">Dunkelgrün</option>
            <option value="LIGHTBLUE">Hellblau</option>
          </select><br><br>
        </details>

        <input type="submit" value="Speichern">
      </form>

      <script>
        // Setzt die Dropdowns automatisch auf die aktuell gespeicherten Werte
        document.getElementById('selDisplayColor').value = ')rawliteral" + displayColorName + R"rawliteral(';
        document.getElementById('selTimeColor').value = ')rawliteral" + timeColorName + R"rawliteral(';
      </script>
    </body>
    </html>
    )rawliteral";

    // Animationen pausieren
    stopRainTask();

  server.send(200, "text/html", page);
}

bool setStopIDByName(String stopName) {
  display->clearScreen();
  drawStaticText("suche", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("Station", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);
  delay(2000);

  String apiURL = "https://v6.bvg.transport.rest/stops?query=" + stopName;

  HTTPClient http;
  http.begin(apiURL);
  int httpCode = http.GET();

  String json = "";
  if(httpCode > 0) {
    json = http.getString();
    Serial.println("Successful API request.");
  } else {
    display->clearScreen();
    drawStaticText("Netzwerk", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, RED, 1);
    drawStaticText("Fehler!", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("Neustart!", 0, PANEL_RES_X * PANEL_CHAIN, ROW_4, ALIGN_CENTER, WHITE, 1);

    Serial.println("Fehler bei der API-Anfrage, HTTP Code: " + String(httpCode));
    return false;
  }
  http.end();

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    display->clearScreen();
    drawStaticText("Daten", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, RED, 1);
    drawStaticText("Fehler!", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("Neustart!", 0, PANEL_RES_X * PANEL_CHAIN, ROW_4, ALIGN_CENTER, WHITE, 1);

    Serial.print("JSON Fehler: " + String(error.c_str()));
    return false;
  }

  JsonObject firstStop = doc[0];
  if (firstStop.isNull()) {
    display->clearScreen();
    drawStaticText("keine", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, RED, 1);
    drawStaticText("Station", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("gefunden!", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);

    Serial.println("Keine Station gefunden");

    delay(4000);
    display->clearScreen();
    drawStaticText("Station", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, WHITE, 1);
    drawStaticText("erneut", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, WHITE, 1);
    drawStaticText("eingeben!", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, WHITE, 1);

    return false;
  }

  selectedStopName = firstStop["name"].as<String>();
  String rawId = firstStop["id"].as<String>();

  selectedStopID = extractStationID(rawId);
  Serial.println("Stop ID: " + selectedStopID);

  float latitude = firstStop["location"]["latitude"];
  float longitude = firstStop["location"]["longitude"];

  if (latitude && longitude) {
    latitudeStop = latitude;
    longitudeStop = longitude;
  }

  display->clearScreen();
  drawStaticText("Station", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, GREEN, 1);
  drawStaticText("gefunden:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, GREEN, 1);
  drawStaticText(selectedStopName, 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, DEPARTURE_COLOR, 1);

  delay(5000);
  display->clearScreen();
  return true;
}

void handleSaveStop() {
  if (server.hasArg("stop")) selectedStopName = server.arg("stop");
  if (server.hasArg("line1")) selectedLine = server.arg("line1");
  if (server.hasArg("line2")) selectedLine2 = server.arg("line2");
  if (server.hasArg("line3")) selectedLine3 = server.arg("line3");
  if (server.hasArg("line4")) selectedLine4 = server.arg("line4");
  if (server.hasArg("timeColor")) timeColorName = server.arg("timeColor");
  if (server.hasArg("displayColor")) displayColorName = server.arg("displayColor");
  if (server.hasArg("offset")) offsetMin = server.arg("offset").toInt();

  if (server.hasArg("replaceFrom1")) replaceFrom1 = server.arg("replaceFrom1");
  if (server.hasArg("replaceTo1")) replaceTo1 = server.arg("replaceTo1");
  if (server.hasArg("replaceFrom2")) replaceFrom2 = server.arg("replaceFrom2");
  if (server.hasArg("replaceTo2")) replaceTo2 = server.arg("replaceTo2");
  if (server.hasArg("replaceFrom3")) replaceFrom3 = server.arg("replaceFrom3");
  if (server.hasArg("replaceTo3")) replaceTo3 = server.arg("replaceTo3");
  if (server.hasArg("replaceFrom4")) replaceFrom4 = server.arg("replaceFrom4");
  if (server.hasArg("replaceTo4")) replaceTo4 = server.arg("replaceTo4");

  // extras
  ads = (selectedLine == "!WERBUNG" );
  if (ads) {
    adsInterval = selectedLine2.toInt();
    selectedLine = "";
    selectedLine2 = "";

  } 
  Serial.println("selectedLine:" + selectedLine);
  discoMode = (selectedLine3 == "!DISCO");
  if (discoMode) {
    discoTime = selectedLine4.toInt();
    selectedLine3 = "";
    selectedLine4 = "";
  }
  
  // Wichtig: Bei Checkboxen ist der Key nur im Request, wenn sie angehakt sind!
  showLine = server.hasArg("showLine");
  showWeatherTime = server.hasArg("weather");

  DEPARTURE_COLOR = getDisplayColorFromString(displayColorName);
  CLOCK_COLOR = getDisplayColorFromString(timeColorName);

  bool success = setStopIDByName(selectedStopName);

  if (!success) {
    server.sendHeader("Location", "/configStop", true);
    server.send(302, "text/plain", "");
    return;
  }

  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Konfiguration abgeschlossen!</title>
  </head>
  <body>
  <h1>Konfiguration erfolgreich abgeschlossen!</h1>
  <p>Das Display übernimmt jetzt die Daten. Du kannst das WLAN jetzt offen lassen und jederzeit Einstellungen ändern.</p>
  <p>Beendest du das WLAN, schließt sich das Setup-Netzwerk. Startest du das Display danach neu, beginnt die Konfiguration wieder von vorne.</p>
  
  <br>
  <form action="/configStop" method="GET">
    <button type="submit">Konfiguration ändern</button>
  </form>
  <br>
  <form action="/closeWLAN" method="POST">
    <button type="submit">WLAN endgültig schließen</button>
  </form>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);

  display->clearScreen();
  drawStaticText("Lade", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, DEPARTURE_COLOR, 1);
  drawStaticText("Daten", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, DEPARTURE_COLOR, 1);

  // Zustand: Setup abgeschlossen, Config-Modus bleibt vorerst aktiv (WLAN offen)
  inConfigMode = true;
  saveSettings();

  setupComplete = true; // Signalisiert Config(), dass die Schleife beendet werden kann
  finishSetup();
}

void deactivateConfigMode() {
  inConfigMode = false;
  saveSettings();

  server.send(200, "text/html",
    "<html><body><h3>WLAN wird geschlossen!</h3><p>Du kannst das Setup-Fenster jetzt schließen.</p></body></html>");
  
  delay(1000);
  WiFi.softAPdisconnect(true);
}

/* ========================================================= */
/* SERVER SETUP & ROUTING
/* ========================================================= */

void setupAP(bool firstStart) {
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", apIP);

  if (firstStart) {
    server.on("/", handleRoot);
  } else {
    // Leitet Nutzer bei erneutem Verbinden sofort zur Station-Config
    server.on("/", handleStopConfig);
  }
  
  server.on("/saveWifi", HTTP_POST, handleSaveWifi);
  server.on("/configStop", handleStopConfig);
  server.on("/saveStop", HTTP_POST, handleSaveStop);
  server.on("/closeWLAN", HTTP_POST, deactivateConfigMode);

  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

/* ========================================================= */
/* HAUPT-CONFIG ABLAUF (Aufruf in setup)
/* ========================================================= */

void Config() {
  bool hasSavedData = loadSettings(); 

  DEPARTURE_COLOR = getDisplayColorFromString(displayColorName);
  CLOCK_COLOR = getDisplayColorFromString(timeColorName);

  // FALL 3B: Daten existieren & WLAN wurde NICHT geschlossen
  if (hasSavedData && inConfigMode) {
    display->clearScreen();
    drawStaticText("Display-WLAN:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, WHITE, 1);
    drawStaticText(espSSID, 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("Passwort:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, WHITE, 1);
    drawStaticText(espPassword, 0, PANEL_RES_X * PANEL_CHAIN, ROW_4, ALIGN_CENTER, RED, 1);

    startAP();
    setupAP(false); // false = Root-Pfad leitet zur Station-Config
    
    // Für 7 Sekunden anzeigen
    delay(7000);

    display->clearScreen();
    drawStaticText("verbinde", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
    drawStaticText("WLAN", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

    if (connectStoredWifi()) {
      finishSetup();
      return; // Config hier erfolgreich beenden, Loop() kann starten!
    } else {
      // Wenn das automatische Verbinden fehlschlägt, normaler Start
      hasSavedData = false;
    }
  }

  // FALL 3A / 4: Kein WLAN konfiguriert oder es wurde manuell geschlossen
  inConfigMode = true; // Setze für diese Session wieder auf Setup-Modus
  saveSettings();

  display->clearScreen();
  drawStaticText("starte", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("SETUP", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);
  delay(2000);

  scanWIFIOptions();

  startAP();
  setupAP(true); // true = Root-Pfad leitet zur WLAN-Auswahl

  display->clearScreen();
  drawStaticText("Display-WLAN:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, WHITE, 1);
  drawStaticText(espSSID, 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
  drawStaticText("Passwort:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, WHITE, 1);
  drawStaticText(espPassword, 0, PANEL_RES_X * PANEL_CHAIN, ROW_4, ALIGN_CENTER, RED, 1);

  setupComplete = false;
  // Warten, bis der Benutzer das Setup im Browser abschließt
  while (!setupComplete) {
    dnsServer.processNextRequest();
    server.handleClient();
    delay(10);
  }
}

/* ========================================================= */
/* HINTERGRUND-VERARBEITUNG (Aufruf in loop)
/* ========================================================= */

void handleBackgroundWLAN() {
  if (inConfigMode) {
    dnsServer.processNextRequest();
    server.handleClient();
  }
}