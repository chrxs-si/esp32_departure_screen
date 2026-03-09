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
String selectedLine2 = "";

float latitudeStop = 52.5170365;
float longitudeStop = 13.3888599;

bool showLine = false;
bool showWeatherTime = true;

String wifiOptionsHTML = "";

/* ========================================================= */

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
    <p>Hallo! Schön das du es hierher geschafft hast. Um die Anzeige zu benutzen ist natürliche eine Internetverbindung nötig. Wähle also bitte unten dein WLAN aus und gebe das Passwort ein. Sollte dein WLAN nicht angezeigt werden, stecke das Display noch einmal vom Strom ab und wieder an. Mache das, wenn möglich, direkt neben dem WLAN-Router.</p>
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

/* ======================= WLAN SPEICHERN ========================= */

void connectToWifi(String ssid, String password) {
  WiFi.mode(WIFI_AP_STA);  // AP bleibt aktiv
  WiFi.begin(ssid.c_str(), password.c_str());
}

void handleSaveWifi() {

  if (server.hasArg("ssid")) selectedSSID = server.arg("ssid");
  if (server.hasArg("password")) selectedPassword = server.arg("password");

  display->clearScreen();
  drawStaticText("verbinde", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

  connectToWifi(selectedSSID, selectedPassword);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 9000) {
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
    drawStaticText("WLAN:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, WHITE, 1);
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
    <h2>Station Konfiguration</h2>
    <p>Super! WLAN haben wir schon einmal. Als nächstes musst du angeben zu welcher Station du die Abfahrten angezeigt bekommen haben möchtest. Gebe dafür einen eindeutigen Teil des Stationsnamens ein, damit die Station gefunden werden kann (Bei "Warschauer Straße", würde z.B. "Warschauer" reichen). Bei Tippfehlern kann die Station leider nicht gefunden werden.</p>
    <p>Du kannst außerdem optional bis zu zwei bestimmte Linie eingeben. Fahren an der Station mehrere Linien, werden nur diese eingegebenen Linien angezeigt. Wenn du hier nichts eingibst, werden alle Linien angezeigt. Hier ein paar Beispiele wie die Linien geschrieben werden müssen: Bus: z.B. "101", "M45", "N2", U-Bahn: z.B. "U1", S-Bahn: z.B. "S3", Tram z.B. "61", "M10", Regio: z.B. "RE1".</p>
    <p>Unter "Linie anzeigen", kannst du einstellen, ob die Linie auf der Abfahrtsanzeige mit angezeigt werden soll. Das nimmt auf der Anzeige viel Platz weg, so dass der Zielbahnhof kaum zu erkennen ist. Sinnvoll ist das z.B. wenn man sich die S41 und S42 anzeigen lassen möchte, da als Ziel hier sowieso nur "Ringbahn" angezeigt wird.</p>
    <p>Das Display zeigt außerdem standardmäßig die aktuelle Uhrzeit und das Wetter an. Wenn du das nicht möchtest, kannst du die entsprechende Option einfach ausstellen.</p>

    <form action="/saveStop" method="POST">

      <label>Station*:</label><br>
      <input type="text" name="stop" required><br><br>

      <label>1. Linie (optional):</label><br>
      <input type="text" name="line"><br><br>

      <label>2. Linie (optional):</label><br>
      <input type="text" name="line2"><br><br>

      <input type="checkbox" name="showLine" unchecked>
      Linie anzeigen<br><br>

      <input type="checkbox" name="weather" checked>
      Wetter und Uhrzeit anzeigen<br><br>

      <input type="submit" value="Speichern">
    </form>
  </body>
  </html>
  )rawliteral"; 

  // TODO: "offset" implementieren. Abfahrt wird nur angezeigt, falls sie länger als Offset ist. Bsp. offset=5 min, Anfahrt in 4min, Abfahrt wird nicht mehr angezeigt
  // TODO: WLAN bleibt offen. Weitere Seite leitet durch BUtton entweder zurück auf Konfigurationsseite oder schaltet eigenes Wlan aus.

  server.send(200, "text/html", page);
}

/* ======================= STATION PRÜFEN ========================= */

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

  // Stop ID extrahieren
  selectedStopID = extractStationID(rawId);
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
  drawStaticText("Station", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, GREEN, 1);
  drawStaticText("gefundne:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, GREEN, 1);
  drawStaticText(selectedStopName, 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, DEPARTURE_COLOR, 1);

  delay(5000);
  display->clearScreen();
  return true;
}

void handleSaveStop() {

  if (server.hasArg("stop")) selectedStopName = server.arg("stop");
  if (server.hasArg("line1")) selectedLine = server.arg("line1");
  if (server.hasArg("line2")) selectedLine2 = server.arg("line2");
  if (server.hasArg("showLine")) showLine = server.arg("showLine");
  if (server.hasArg("weather")) showWeatherTime = server.arg("weather");

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

  drawStaticText("Lade", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, DEPARTURE_COLOR, 1);
  drawStaticText("Daten", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, DEPARTURE_COLOR, 1);
}

/* ======================= AP START ========================= */

void startAP() {
  display->clearScreen();
  drawStaticText("starte", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("WLAN", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);

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
  drawStaticText("starte", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
  drawStaticText("SETUP", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);
  delay(2000);

  scanWIFIOptions();
  startAP();
  setupAP();

  display->clearScreen();
  drawStaticText("WLAN:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_1, ALIGN_CENTER, WHITE, 1);
  drawStaticText(espSSID, 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
  drawStaticText("Passwort:", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, WHITE, 1);
  drawStaticText(espPassword, 0, PANEL_RES_X * PANEL_CHAIN, ROW_4, ALIGN_CENTER, RED, 1);

  while (inConfigMode) {
    dnsServer.processNextRequest();
    server.handleClient();
  }

  WiFi.softAPdisconnect(true);
}
