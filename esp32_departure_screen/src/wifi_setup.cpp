#include "wifi_setup.h"
#include "display.h"

#define DNS_PORT 53

WebServer server(80);
DNSServer dnsServer;

String espSSID = "SETUP-" + String(random(100, 1000));
String espPassword = String(random(10000000, 100000000));

// Globale Variablen
bool inConfigMode = false;

String selectedSSID = "";
String selectedPassword = "";

String selectedStopName = "";
String selectedStopID = "";
String selectedLine = "";

float latitudeStop = 52.5170365;
float longitudeStop = 13.3888599;

bool showWeatherTime = true;

String wifiOptionsHTML = "";

/* ========================================================= */

void scanWIFIOptions() {
  wifiOptionsHTML = "";
  
  display->clearScreen();
  drawStaticText("scanne", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

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

/* ======================= SEITE 1 ========================= */

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
    <p>Bitte WLAN auswählen und Passwort eingeben.</p>

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

/* ======================= WLAN SPEICHERN ========================= */

void connectToWifi(String ssid, String password) {
  WiFi.mode(WIFI_AP_STA);  // AP bleibt aktiv
  WiFi.begin(ssid.c_str(), password.c_str());
}

void handleSaveWifi() {

  if (server.hasArg("ssid")) selectedSSID = server.arg("ssid");
  if (server.hasArg("password")) selectedPassword = server.arg("password");

  display->clearScreen();
  drawStaticText("verbinde", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  connectToWifi(selectedSSID, selectedPassword);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 12000) {
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    display->clearScreen();
    drawStaticText("WLAN", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, RED, 1);
    drawStaticText("Fehler!", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, RED, 1);

    delay(4000);
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
    return;
  }

  display->clearScreen();
  drawStaticText("WLAN", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, GREEN, 1);
  drawStaticText("Verbunden!", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, GREEN, 1);

  server.sendHeader("Location", "/configStop", true);
  server.send(302, "text/plain", "");
}

/* ======================= SEITE 2 ========================= */

void handleStopConfig() {

  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Stationen Setup</title>
  </head>
  <body>
    <h2>Stationen Konfiguration</h2>

    <form action="/saveStop" method="POST">

      <label>Station*:</label><br>
      <input type="text" name="stop" required><br><br>

      <label>Linie (optional):</label><br>
      <input type="text" name="line"><br><br>

      <input type="checkbox" name="weather" checked>
      Wetter und Uhrzeit anzeigen<br><br>

      <input type="submit" value="Speichern">
    </form>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

/* ======================= STATION PRÜFEN ========================= */

bool setStopIDByName(String stopName) {
    display->clearScreen();
  drawStaticText("suche", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("Station", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);
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
    display->clearScreen();
    drawStaticText("Netzwerk", 0, PANEL_RES_X, ROW_1, ALIGN_CENTER, RED, 1);
    drawStaticText("Fehler!", 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("Neustart!", 0, PANEL_RES_X, ROW_4, ALIGN_CENTER, WHITE, 1);

    Serial.println("Fehler bei der API-Anfrage, HTTP Code: " + String(httpCode));
    return false;
  }

  http.end();

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    display->clearScreen();
    drawStaticText("Daten", 0, PANEL_RES_X, ROW_1, ALIGN_CENTER, RED, 1);
    drawStaticText("Fehler!", 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("Neustart!", 0, PANEL_RES_X, ROW_4, ALIGN_CENTER, WHITE, 1);

    Serial.print("JSON Fehler: " + String(error.c_str()));
    return false;
  }

  JsonObject firstStop = doc[0];
  if (firstStop.isNull()) {
    display->clearScreen();
    drawStaticText("keine", 0, PANEL_RES_X, ROW_1, ALIGN_CENTER, RED, 1);
    drawStaticText("Station", 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("gefunden!", 0, PANEL_RES_X, ROW_3, ALIGN_CENTER, RED, 1);
    drawStaticText("Neustart!", 0, PANEL_RES_X, ROW_4, ALIGN_CENTER, WHITE, 1);

    Serial.println("Keine Station gefunden");
    return false;
  }

  String rawId = firstStop["id"].as<String>();

  int firstColon  = rawId.indexOf(':');
  int secondColon = rawId.indexOf(':', firstColon + 1);
  int lastColon   = rawId.lastIndexOf(':');
  int secondLastColon = rawId.lastIndexOf(':', lastColon - 1);

  selectedStopID = rawId.substring(secondColon + 1, secondLastColon);
  Serial.println("Stop ID: " + selectedStopID);

  // Latitude & Longitude für das Wetter
  float latitude = firstStop["location"]["latitude"];
  float longitude = firstStop["location"]["longitude"];

  if (latitude && longitude) {
    Serial.println("Latitude: " + String(latitude, 6));
    Serial.println("Longitude: " + String(longitude, 6));
    latitudeStop = latitude;
    longitudeStop = longitude;
  } else {
    Serial.println("Keine gültigen Koordinaten gefunden.");
  }

  http.end();

  display->clearScreen();
  drawStaticText("Setup", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, GREEN, 1);
  drawStaticText("fertig!", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, GREEN, 1);

  delay(5000);
  display->clearScreen();
  return true;
}

void handleSaveStop() {

  if (server.hasArg("stop")) selectedStopName = server.arg("stop");
  if (server.hasArg("line")) selectedLine = server.arg("line");
  showWeatherTime = server.hasArg("weather");

  bool success = setStopIDByName(selectedStopName);

  if (!success) {
    server.sendHeader("Location", "/configStop", true);
    server.send(302, "text/plain", "");
    return;
  }

  server.send(200, "text/html",
    "<html><body><h3>Station gefunden! :)</h3>"
    "<a href='/configStop'>Zurück</a></body></html>");

  inConfigMode = false;

  drawStaticText("Lade", 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, DEPARTURE_COLOR, 1);
  drawStaticText("Daten", 0, PANEL_RES_X, ROW_3, ALIGN_CENTER, DEPARTURE_COLOR, 1);
}

/* ======================= AP START ========================= */

void startAP() {
  display->clearScreen();
  drawStaticText("starte", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(espSSID.c_str(), espPassword.c_str());
  delay(500);
}

/* ======================= SETUP AP ========================= */

void setupAP() {

  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/saveWifi", HTTP_POST, handleSaveWifi);
  server.on("/configStop", handleStopConfig);
  server.on("/saveStop", HTTP_POST, handleSaveStop);

  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

/* ======================= CONFIG START ========================= */

void Config() {

  inConfigMode = true;

  display->clearScreen();
  drawStaticText("starte", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("SETUP", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);
  delay(2000);

  scanWIFIOptions();
  startAP();
  setupAP();

  display->clearScreen();
  drawStaticText("WLAN:", 0, PANEL_RES_X, ROW_1, ALIGN_CENTER, WHITE, 1);
  drawStaticText(espSSID, 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, RED, 1);
  drawStaticText("Passwort:", 0, PANEL_RES_X, ROW_3, ALIGN_CENTER, WHITE, 1);
  drawStaticText(espPassword, 0, PANEL_RES_X, ROW_4, ALIGN_CENTER, RED, 1);

  while (inConfigMode) {
    dnsServer.processNextRequest();
    server.handleClient();
  }

  WiFi.softAPdisconnect(true);
}
