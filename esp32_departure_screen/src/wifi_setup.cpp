#include "wifi_setup.h"
#include "display.h"

#define DNS_PORT 53

WebServer server(80);
DNSServer dnsServer;
String espSSID = "ESP32-SETUP";
String espPassword = "10625145";

// Globale Variablen
bool inConfigMode = false;
String selectedSSID = "";
String selectedPassword = "";
String selectedStopName = "";
String selectedStopID = "";
String selectedLine = "";
String latitudeStop = "";
String longitudeStop = "";
bool showWeatherTime = true;

String wifiOptionsHTML = "";


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


void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 Setup</title>
  </head>
  <body>
    <h2>Display Konfiguration</h2>
    <p>Bitte WLAN auswählen und Passwort des WLAN's angeben.</p>
    <p>Unter "Haltestelle" den eindeutigen Namens-Anfang der Haltestelle angeben. In der Regel reichen die ersten paar Buchstaben, damit die Hatestelle eindeutig gefunden werden kann.</p>
    <p>Unter "Linie" die Linie angeben, falls gewünscht (z.B. "U1"(U-Bahn), "S1"(S-Bahn), "61"(Straßenbahn), "100"(Bus), etc.).</p>

    <form action="/save" method="POST">
      <label>WLAN Netzwerk*:</label><br>
      <select name="ssid">
  )rawliteral";

  page += wifiOptionsHTML;

  page += R"rawliteral(
      </select><br><br>

      <label>WLAN Passwort*:</label><br>
      <input type="password" name="password"><br><br>

      <label>Haltestelle*:</label><br>
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


void connectToWifi(String SSID, String password) {
    WiFi.mode(WIFI_STA);
  WiFi.begin(SSID.c_str(), password.c_str());

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 15000) {
    delay(500);
  }
}


bool setStopIDByName(String stopName) {
  display->clearScreen();
  drawStaticText("suche", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("Haltestelle", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);
  delay(2000);

  // send API request to get stopID
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

  // parse Json
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
    drawStaticText("Haltestelle", 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("gefunden!", 0, PANEL_RES_X, ROW_3, ALIGN_CENTER, RED, 1);
    drawStaticText("Neustart!", 0, PANEL_RES_X, ROW_4, ALIGN_CENTER, WHITE, 1);

    Serial.println("Keine Haltestelle gefunden");
    return false;
  }

  String rawId = firstStop["id"].as<String>();

  int lastColon = rawId.lastIndexOf(':');
  selectedStopID = rawId.substring(rawId.lastIndexOf(':', lastColon - 1) + 1, lastColon);
  Serial.println("Stop ID: " + selectedStopID);

  // Latitude & Longitude für das Wetter
  float latitude = firstStop["location"]["latitude"];
  float longitude = firstStop["location"]["longitude"];

  String latitudeStop = String(latitude, 6);
  String longitudeStop = String(longitude, 6);

  http.end();

  return true;
}


void handleSave() {

  display->clearScreen();
  drawStaticText("speichere", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("Daten", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  if (server.hasArg("ssid")) selectedSSID = server.arg("ssid");
  if (server.hasArg("password")) selectedPassword = server.arg("password");
  if (server.hasArg("stop")) selectedStopName = server.arg("stop");
  if (server.hasArg("line")) selectedLine = server.arg("line");

  showWeatherTime = server.hasArg("weather");

  server.send(200, "text/html",
              "<html><body><h3>Daten werden gespeichert</h3></body></html>");

  delay(2000);

  server.send(200, "text/html",
              "<html><body><h3>Verbinde mit WLAN</h3></body></html>");

  display->clearScreen();
  drawStaticText("verbinde", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  WiFi.softAPdisconnect(true);

  connectToWifi(selectedSSID, selectedPassword);

  inConfigMode = false;

  delay(500);

  if (WiFi.status() != WL_CONNECTED) {
    display->clearScreen();
    drawStaticText("Fehler im", 0, PANEL_RES_X, ROW_1, ALIGN_CENTER, RED, 1);
    drawStaticText("Setup!", 0, PANEL_RES_X, ROW_2, ALIGN_CENTER, RED, 1);
    drawStaticText("Neustart!", 0, PANEL_RES_X, ROW_4, ALIGN_CENTER, WHITE, 1);

    delay(3000);
    Config();  // Neustart Konfiguration bei Fehler
    return;
  }

  display->clearScreen();
  drawStaticText("Verbunden!", 0, PANEL_RES_X, CENTER, ALIGN_CENTER, GREEN, 1);
  server.send(200, "text/html",
            "<html><body><h3>Konfiguration erfolgreich!</h3></body></html>");

  delay(2000);

  bool success = setStopIDByName(selectedStopName);
  if (!success) {
    delay(3000);
    Config(); // Neustart Konfiguration bei Fehler
    return;
  }
}


void startAP() {
  display->clearScreen();
  drawStaticText("starte", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(espSSID.c_str(), espPassword.c_str());
  delay(500);
}


void setupAP() {

  IPAddress apIP = WiFi.softAPIP();

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);

  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}


void Config() {

  inConfigMode = true;

  display->clearScreen();
  drawStaticText("STARTE", 0, PANEL_RES_X, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
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
}
