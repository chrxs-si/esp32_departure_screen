#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Panel Konfiguration
#define PANEL_RES_X 64      // Breite des Panels
#define PANEL_RES_Y 32      // Höhe des Panels
#define PANEL_CHAIN 1       // Anzahl der Panels (verkettet)

MatrixPanel_I2S_DMA *display = nullptr;

// Grundfarben
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

//Wlan & Config-Website
WebServer server(80);
String selectedSSID = "";
String selectedPassword = "";
String stop1 = "";
String line1 = "";
int maxLines1 = 1;
int minOffset1 = 0;
String stop2 = "";
String line2 = "";
int maxLines2 = 1;
int minOffset2 = 0;

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
      Max-Zeilen: <select name="maxLines1">
)rawliteral";
}

// Funktion zum Zeichnen von Text
void displayText(String text, int line, uint16_t color) {
  int16_t x1, y1;
  uint16_t w, h;

  // Textgröße einstellen, z.B. 1
  display->setTextSize(1);
  display->setTextColor(color);

  // Berechne die Textbox
  display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int y = 0;
  int x = 0;

  if(line == -1) {
    // Zentrieren
    x = (PANEL_RES_X - w) / 2;
    y = (PANEL_RES_Y - h) / 2;
  } else if(line >= 0 && line <= 3) {
    // In einer von 4 Zeilen
    int lineHeight = PANEL_RES_Y / 4;
    y = line * lineHeight + (lineHeight - h) / 2; // vertikal zentriert in der Zeile
    x = (PANEL_RES_X - w) / 2; // optional horizontal zentriert
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
// Funktion zum Einrichten des WLAN Access Points

void setupWiFiAP(const char* ssid, const char* password) {
  // WLAN im AP-Modus starten
  WiFi.mode(WIFI_AP_STA);
  bool result = WiFi.softAP(ssid, password);
}


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
  return options;
}

void handleRoot() {
  String wifiOptions = getWiFiOptions();
  String page = R"rawliteral(
<html>
  <body>
    <h3>WLAN Auswahl:</h3>
    <form action="/save" method="POST">
      WLAN: <select name="ssid">)" + wifiOptions + R"rawliteral(</select><br>
      Passwort: <input type="password" name="password"><br><br>

      <h3>1. Haltestelle:</h3>
      Haltestelle-ID: <input type="text" name="stop1"><br>
      Linie: <input type="text" name="line1"><br>
      Max-Zeilen: <select name="maxLines1">)rawliteral";

  // Dropdown für Max-Zeilen 1-4
  for(int i=1;i<=4;i++) page += "<option value=\"" + String(i) + "\">" + String(i) + "</option>";
  page += R"rawliteral(</select><br>Minuten-Offset: <select name="minOffset1">)rawliteral";
  for(int i=0;i<=15;i++) page += "<option value=\"" + String(i) + "\">" + String(i) + "</option>";
  page += R"rawliteral(</select><br><br>

      <h3>2. Haltestelle:</h3>
      Haltestelle-ID: <input type="text" name="stop2"><br>
      Linie: <input type="text" name="line2"><br>
      Max-Zeilen: <select name="maxLines2">)rawliteral";

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
  maxLines1 = server.arg("maxLines1").toInt();
  minOffset1 = server.arg("minOffset1").toInt();

  stop2 = server.arg("stop2");
  line2 = server.arg("line2");
  maxLines2 = server.arg("maxLines2").toInt();
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

void setup() {
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);

  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(100);

  Serial.begin(115200);
  
  Intro();
  loadingTransition();
  Config();
}



void loop() {
  server.handleClient();
}