#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include "time.h"
#include "transport_data.h"
#include "display.h"
#include "weather_icons.h"
#include "wifi_setup.h"
#include "weather_data.h"
#include "clock.h"
#include "extras.h"

//Wlan & Config-Website --> Find stop id: https://v6.bvg.transport.rest/stops?query=Leibnizstr./B
// API for departures: https://v6.vbb.transport.rest/stops/900024208/departures
//Useless Facts API: https://uselessfacts.jsph.pl/api/v2/facts/random?language=de

void updateTemprature(int temp) {
    uint16_t textColor;

  // Farbwahl je nach Temperatur
  if (temp <= 0) {
    textColor = LIGHTBLUE;
  } else if (temp < 10) {
    textColor = BLUE;
  } else if (temp < 20) {
    textColor = GREEN;
  } else if (temp < 30) {
    textColor = ORANGE;
  } else {
    textColor = RED;
  }

  drawStaticText(String(temp) + String("C"), PANEL_RES_X * PANEL_CHAIN - 35, PANEL_RES_X * PANEL_CHAIN - 11, ROW_1, ALIGN_RIGHT, textColor, 1);
}

void updateBrightness(bool is_day) {
   if (is_day) {
    display->setBrightness8(100);
  } else {
    display->setBrightness8(60);
  }
}

void updateWeather() {
  Serial.print("update Weather.");

  String weatherJson = fetchWeatherJson(latitudeStop, longitudeStop);
  WeatherData data = parseWeatherJson(weatherJson, data) ? data : WeatherData();

  updateTemprature((int)round(data.temperature_2m));
  updateWeatherIcon(data.weather_code, data.is_day);

  updateBrightness(data.is_day);
}

void updateSystemTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now;
  while (time(&now) < 100000) {
    delay(100);
  }
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();
}

void Intro() {
    return;

    display->clearScreen();
    drawStaticText("HALLO!", 0, PANEL_RES_X * PANEL_CHAIN, CENTER, ALIGN_CENTER, WHITE, 1);

    delay(3000);

    display->clearScreen();
    drawStaticText("Toll,", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_ABOVE, ALIGN_CENTER, WHITE, 1);
    drawStaticText("dass du da bist!", 0, PANEL_RES_X * PANEL_CHAIN, CENTER_BELOW, ALIGN_CENTER, WHITE, 1);
    delay(5000);

    display->clearScreen();
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("starte setup...");
  initDisplay();

  randomSeed(esp_random()); 

  Intro();
  
  // Startet den gesamten Konfigurations- und Verbindungsablauf
  Config();

  Serial.println("setup fertig.");
}

// --- FINISH SETUP ---
void finishSetup() {
  // Warten auf WLAN Verbindung
  int retrys = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("No wifi connection yet...");

    if (retrys > 15) {
      Serial.println("WLAN Verbindung fehlgeschlagen. Starte Config neu.");
      Config(); 
      return; // Verhindert weitere Ausführung in dieser Instanz
    }
    retrys += 1;
  }
  Serial.println("WiFi connected.");

  display->clearScreen();

  updateSystemTime();
  updateWeather();
  updateDepartures();
}

int lastUpdate = millis();
int departureSecondsCounter = 999999;
int weatherSecondsCounter = 999999;
int SystemTimeSecondsCounter = 999999;
//extras
int adsTimer = 999999;

void loop() {

  handleBackgroundWLAN();

  unsigned long now = millis();

  // Sekunden hochzählen
  if (now - lastUpdate > 1000) {
    lastUpdate = now;

    departureSecondsCounter += 1;
    weatherSecondsCounter += 1;
    SystemTimeSecondsCounter += 1;
    if (ads) adsTimer += 1;

    if (showWeatherTime) {
      drawTime();
    }
  }

  // extras
  if (ads && adsTimer > adsInterval) {
    adsTimer = 0;
    departureSecondsCounter = 999999;
    weatherSecondsCounter = 999999;
    runAd();
  }

  if (discoMode) {
    adsTimer += discoTime;
    runDisco(discoTime * 1000);
    return;
  }

  if (departureSecondsCounter >= 6) { // alle 6 Sekunden
    departureSecondsCounter = 0;
    updateDepartures();
  }

  if (weatherSecondsCounter >= 600) { // alle 6 Minuten
    weatherSecondsCounter = 0;
    if (showWeatherTime) {
      updateWeather();
    }
  }

  if (SystemTimeSecondsCounter >= 21000) { // alle 6 Stunden
    SystemTimeSecondsCounter = 0;
    updateSystemTime(); // Systemzeit alle 6 Stunden aktualisieren, um Drift zu vermeiden
  }
}