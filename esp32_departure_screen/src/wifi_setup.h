#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Globale Konfigurationswerte
extern bool inConfigMode;

extern String selectedSSID;
extern String selectedPassword;

extern String selectedStopName;
extern String selectedStopID;
extern String selectedLine;
extern String selectedLine2;

extern float latitudeStop;
extern float longitudeStop;

extern bool showLine;
extern bool showWeatherTime;

// Hauptfunktionen
void Config();
void connectToWifi(String ssid, String password);
void scanWIFIOptions();

#endif
