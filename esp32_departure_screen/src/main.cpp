#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Panel Konfiguration
#define PANEL_RES_X 64      // Breite des Panels
#define PANEL_RES_Y 32      // Höhe des Panels
#define PANEL_CHAIN 1       // Anzahl der Panels (verkettet)

MatrixPanel_I2S_DMA *display = nullptr;

// Grundfarben
uint16_t DEPARTURE_COLOR  = display->color565(255, 165, 0);
uint16_t BLACK   = display->color565(0, 0, 0);
uint16_t WHITE   = display->color565(255, 255, 255);
uint16_t RED     = display->color565(255, 0, 0);
uint16_t GREEN   = display->color565(0, 255, 0);
uint16_t BLUE    = display->color565(0, 0, 255);
uint16_t YELLOW  = display->color565(255, 255, 0);
uint16_t CYAN    = display->color565(0, 255, 255);
uint16_t MAGENTA = display->color565(255, 0, 255);
uint16_t ORANGE  = display->color565(255, 165, 0);
uint16_t PURPLE  = display->color565(128, 0, 128);
uint16_t PINK    = display->color565(255, 192, 203);
uint16_t BROWN   = display->color565(165, 42, 42);
uint16_t GRAY    = display->color565(128, 128, 128);
uint16_t LIGHTGRAY = display->color565(192, 192, 192);
uint16_t DARKGREEN = display->color565(0, 100, 0);
uint16_t LIGHTBLUE = display->color565(173, 216, 230);

//Wlan & Config-Website --> Find stop id: https://v6.bvg.transport.rest/stops?query=Leibnizstr./B
WebServer server(80);
String selectedSSID = "MotivNet";
String selectedPassword = "motivoli5";
String stop1 = "";
String line1 = "";
int maxColumns1 = 1;
int minOffset1 = 0;
String stop2 = "";
String line2 = "";
int maxColumns2 = 1;
int minOffset2 = 0;

//parsen & Anzeigen
#define MAX_DEPARTURES 8 //Max departures requested from API
#define MAX_LINE_LEN   3
#define MAX_DEST_LEN   6
struct Departure {
  char line[MAX_LINE_LEN];
  char destination[MAX_DEST_LEN];
  int minutes;
  int delay;
};
Departure departures[MAX_DEPARTURES];

String generateSetupPage(String wifiOptions) {
  return R"rawliteral(
<html>
  <head>
    <title>ESP32 Setup</title>
  </head>
  <body>
    <h3>WLAN Auswahl:</h3>
    <form action="/save" method="POST">
      WLAN: <select name="ssid">)" + wifiOptions + R"rawliteral(</select><br><br>

      <h3>1. Haltestelle:</h3>
      Haltestelle: <input type="text" name="stop1"><br>
      Linie: <input type="text" name="line1"><br>
      Max-Zeilen: <select name="maxColumns1">
)rawliteral";
}


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

String normalizeUmlauts(String text) {
  text.replace("ä", "ae");
  text.replace("ö", "oe");
  text.replace("ü", "ue");
  text.replace("Ä", "Ae");
  text.replace("Ö", "Oe");
  text.replace("Ü", "Ue");
  text.replace("ß", "ss");
  return text;
}

// Funktion zum Zeichnen von Text mit optionaler horizontaler Ausrichtung
enum TextAlign {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
};
void displayText(String text, int line, uint16_t color, TextAlign align = ALIGN_CENTER) {
  int16_t x1, y1;
  uint16_t w, h;

  display->setTextSize(1);
  display->setTextColor(color);

  text = normalizeUmlauts(text);

  // Textbox berechnen
  display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int x = 0;
  int y = 0;

  // Vertikale Position
  if (line == -1) {
    // Komplett zentriert
    y = (PANEL_RES_Y - h) / 2;
  } else if (line >= 0 && line <= 3) {
    int lineHeight = PANEL_RES_Y / 4;
    y = line * lineHeight + (lineHeight - h) / 2;
  }

  // Horizontale Ausrichtung
  switch (align) {
    case ALIGN_LEFT:
      x = 0;
      break;

    case ALIGN_CENTER:
      x = (PANEL_RES_X - w) / 2;
      break;

    case ALIGN_RIGHT:
      x = PANEL_RES_X - w;
      break;
  }

  display->setCursor(x, y);
  display->print(text);
}


void Intro() {
  const char* greetings[] = { "Hallo!", "Hello!", "Bonjour!","Shalom!","Salam!","Hej!","Hei!","Ciao!","hola!","MOIN!"
  };
  for (int i = 0; i < sizeof(greetings) / sizeof(greetings[0]); i++) {
    display->fillScreen(BLACK);
    displayText(greetings[i], -1, WHITE);
    delay(600);
  }
  delay(1000);
  displayText("MOIN!", -1, RED);
  delay(1000);
  display->fillScreen(BLACK);
  delay(500);
  displayText("Digitale", 0, RED);
  delay(500);
  displayText("Abfahrts-", 1, GREEN);
  delay(500);
  displayText("Anzeige", 2, BLUE);
  delay(500);
  displayText("by Chrissi", 3, YELLOW);
  delay(4000);
}


void loadingTransition(int steps = 70) {
  // Schritt 1: Zufällige bunte Rechtecke
  for (int i = 0; i < steps; i++) {
    int rectWidth  = random(5, 20);  // zufällige Breite
    int rectHeight = random(5, 20);  // zufällige Höhe
    int x = random(0, PANEL_RES_X - rectWidth);
    int y = random(0, PANEL_RES_Y - rectHeight);

    // Zufällige Farbe
    uint8_t r = random(0, 256);
    uint8_t g = random(0, 256);
    uint8_t b = random(0, 256);
    uint16_t color = display->color565(r, g, b);

    display->fillRect(x, y, rectWidth, rectHeight, color);
    delay(40); // Geschwindigkeit des Effekts
  }

  // Schritt 2: Schwarzes Rechteck aus der Mitte
  int maxWidth = PANEL_RES_X;
  int maxHeight = PANEL_RES_Y;
  int centerX = PANEL_RES_X / 2;
  int centerY = PANEL_RES_Y / 2;

  for (int i = 0; i <= max(maxWidth, maxHeight); i += 2) {
    int x = centerX - i / 2;
    int y = centerY - i / 2;
    int w = i;
    int h = i;

    // Begrenzung auf Panelgröße
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > PANEL_RES_X) w = PANEL_RES_X - x;
    if (y + h > PANEL_RES_Y) h = PANEL_RES_Y - y;

    display->fillRect(x, y, w, h, BLACK);
    delay(15); // Geschwindigkeit des "Schwarzes Rechteck wächst" Effekts
  }
}

#pragma region WLAN Config Funktionen

// Funktion zum Starten des Setup-WLANs
void startSetupAP() {
  const char* ssid = "ESP-Setup";
  const char* password = "01099";

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password); 
  Serial.println("Setup-AP gestartet!");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  display->fillScreen(BLACK);
  displayText("Setup:", -1, PURPLE);
  delay(500);
  display->fillScreen(BLACK);
  displayText("WLAN:", 0, PURPLE);
  displayText(ssid, 1, PURPLE);
  displayText("Passwort:", 2, PURPLE);
  displayText(password, 3, PURPLE);
}

String getWiFiOptions() {
  WiFi.scanDelete();
  delay(100);

  int n = WiFi.scanNetworks();
  String options = "";
  for(int i = 0; i < n; i++) {
    options += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>\n";
  }
  Serial.println("Gefundene Netzwerke: " + String(n));
  Serial.println("Options: " + String(options));
  return options;
}

void handleRoot() {
  Serial.println("handle Root");

  String wifiOptions = getWiFiOptions();
  String page = R"rawliteral(
<html>
  <body>
    <h3>WLAN Auswahl:</h3>
    <form action="/save" method="POST">
      WLAN: <select name="ssid">)rawliteral";
  page += wifiOptions;
  page += R"rawliteral(</select><br>
      Passwort: <input type="password" name="password"><br><br>

      <h3>1. Haltestelle:</h3>
      Haltestelle-ID: <input type="text" name="stop1"><br>
      Linie: <input type="text" name="line1"><br>
      Max-Zeilen: <select name="maxColumns1">)rawliteral";

  // Dropdown für Max-Zeilen 1-4
  for(int i=1;i<=4;i++) page += "<option value=\"" + String(i) + "\">" + String(i) + "</option>";
  page += R"rawliteral(</select><br>Minuten-Offset: <select name="minOffset1">)rawliteral";
  for(int i=0;i<=15;i++) page += "<option value=\"" + String(i) + "\">" + String(i) + "</option>";
  page += R"rawliteral(</select><br><br>

      <h3>2. Haltestelle:</h3>
      Haltestelle-ID: <input type="text" name="stop2"><br>
      Linie: <input type="text" name="line2"><br>
      Max-Zeilen: <select name="maxColumns2">)rawliteral";

  for(int i=1;i<=4;i++) page += "<option value=\"" + String(i) + "\">" + String(i) + "</option>";
  page += R"rawliteral(</select><br>Minuten-Offset: <select name="minOffset2">)rawliteral";
  for(int i=0;i<=15;i++) page += "<option value=\"" + String(i) + "\">" + String(i) + "</option>";
  page += R"rawliteral(</select><br><br>

      <input type="submit" value="Setup">
    </form>
  </body>
</html>
)rawliteral";

  server.send(200, "text/html", page);
}

// Webserver POST-Handler: Daten speichern und AP schließen
void handleSave() {
  selectedSSID = server.arg("ssid");
  selectedPassword = server.arg("password");

  stop1 = server.arg("stop1");
  line1 = server.arg("line1");
  maxColumns1 = server.arg("maxColumns1").toInt();
  minOffset1 = server.arg("minOffset1").toInt();

  stop2 = server.arg("stop2");
  line2 = server.arg("line2");
  maxColumns2 = server.arg("maxColumns2").toInt();
  minOffset2 = server.arg("minOffset2").toInt();

  server.send(200, "text/html", "<html><body><h3>Setup abgeschlossen! ESP verbindet sich nun...</h3></body></html>");

  delay(1500); // Kurze Pause, damit der Client die Nachricht sieht

  // AP abschalten
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  // Mit ausgewähltem WLAN verbinden
  WiFi.begin(selectedSSID.c_str(), selectedPassword.c_str());
  Serial.println("Verbindung wird hergestellt...");
  int attempts = 0;
  while(WiFi.status() != WL_CONNECTED && attempts < 20){
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("\nVerbunden!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFehler beim Verbinden mit dem WLAN!");
  }
}

void Config() {
  startSetupAP();

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);

  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send(302, "text/html", "");
  });

  server.begin();
  Serial.println("Webserver gestartet!");
}

#pragma endregion WLAN Config Funktionen

String getDeparturesJson(String stopID, String lineFilter, int maxColumns) {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Nicht mit WLAN verbunden!");
    String json = "{\"message\":\"Wifi error\"}";
    return json;
  }

  String apiURL = "https://v6.bvg.transport.rest/stops/" + stopID + "/departures?results=" + MAX_DEPARTURES + "&duration=30";
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
        result[count].line[MAX_LINE_LEN - 1] = '\0';

        strncpy(result[count].destination, dest, MAX_DEST_LEN - 1);
        result[count].destination[MAX_DEST_LEN - 1] = '\0';

        result[count].minutes = isoToRelativeMinutes(when);
        result[count].delay = dep["delay"].isNull() ? 0 : dep["delay"].as<int>() / 60;

        count++;
    }

    return count;
}

void updateDepartures() {
  Serial.println("Aktualisiere Abfahrten...");
  Serial.println("get Departures...");
  String json = getDeparturesJson(stop1, line1, maxColumns1);
  Serial.println("parse Departures...");
  int numDepartures = parseDepartures(json, line1.c_str(), departures, MAX_DEPARTURES);
  Serial.println("numDepartures: " + numDepartures);

  Serial.println("Update Display...");
  display->fillScreen(BLACK);
  int coloums = min(min(numDepartures, maxColumns1), 3);

  for(int i=0; i<coloums; i++) {
    String lineInfo = String(departures[i].line) + " " + String(departures[i].destination);
    Serial.println("Display line " + String(i) + ": " + lineInfo);
    displayText(lineInfo, i, DEPARTURE_COLOR, ALIGN_LEFT);
    displayText(String(departures[i].minutes + departures[i].delay), i, DEPARTURE_COLOR, ALIGN_RIGHT);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("starte setup...");

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);

  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(100);
  
  //Intro();
  loadingTransition();
  if (true) {
    selectedSSID = "MotivNet";
    selectedPassword = "motivoli5";
    stop1 = "900024208";
    line1 = "101";
    maxColumns1 = 4;
    minOffset1 = 0;

    WiFi.begin(selectedSSID.c_str(), selectedPassword.c_str());
  }
  else {
    Config();
  }
  Serial.println("setup fertig.");

  //wait for wifi connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("No wifi connection yet...");
  }
  Serial.println("WiFi connected.");
  
  // Set systemtime and timezone to CET/CEST
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now;
  while (time(&now) < 100000) {
    delay(100);
  }
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();

  // loop
  while (true) {

    server.handleClient();
    updateDepartures();
    delay(10000);
  }
}



void loop() {

}