#include "wifi_setup.h"
#include "display.h"

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

void connectToWiFi(const String& ssid, const String& password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Verbinde mit WLAN...");
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
  connectToWiFi(selectedSSID, selectedPassword);
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