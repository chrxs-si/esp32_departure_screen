#include <WiFi.h>
#include <WebServer.h>

extern WebServer server;
extern String selectedSSID;
extern String selectedPassword;

extern String stop1;
extern String line1;
extern int maxColumns1;
extern int minOffset1;
extern String stop2;
extern String line2;
extern int maxColumns2;
extern int minOffset2;

void connectToWiFi(const String& ssid, const String& password);

void Config();