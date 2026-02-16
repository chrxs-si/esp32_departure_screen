#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>

// Globale Konfigurationswerte
extern String selectedSSID;
extern String selectedPassword;
extern String selectedStopID;
extern String selectedLine;
extern float latitudeStop;
extern float longitudeStop;
extern bool showWeatherTime;

// Hauptfunktionen
void Config();
void startAP();
void setupAP();
void connectToWifi(String SSID, String password);
void scanWIFIOptions();

#endif
