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

//Wlan & Config-Website --> Find stop id: https://v6.bvg.transport.rest/stops?query=Leibnizstr./B
// API for departures: https://v6.vbb.transport.rest/stops/900024208/departures
//Useless Facts API: https://uselessfacts.jsph.pl/api/v2/facts/random?language=de

ScrollingText *line1text;
Clock myClock;

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

  drawStaticText(String(temp) + String("C"), PANEL_RES_X - 35, PANEL_RES_X - 11, ROW_1, ALIGN_RIGHT, textColor, 1);
}

void updateBrightness(bool is_day) {
   if (is_day) {
    display->setBrightness8(100);
  } else {
    display->setBrightness8(50);
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

void updateScrollingText() {

}

void setup() {
  Serial.begin(115200);
  Serial.println("starte setup...");
  initDisplay();

  randomSeed(esp_random()); 

  //loadingTransition(70);
  if (false) {
    selectedStopID = "900001201";
    selectedLine = "S42";
    selectedSSID = "MotivNet";
    selectedPassword = "motivoli5";

    connectToWifi(selectedSSID, selectedPassword);
    delay(1000);
    updateWeather();
    updateDepartures();
  }
  else {
    Config();
  }
  Serial.println("setup fertig.");

  //wait for wifi connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print("No wifi connection yet...");
  }
  Serial.println("WiFi connected.");

  //line1text = new ScrollingText("Hallo Kai!! Es ist bald 15 Uhr!", 0, PANEL_RES_X, ROW_4, PURPLE, 1, 80, 8);
  //line1text->start();

  if (showWeatherTime) {
    myClock.start();
  }
}

int lastUpdate = millis();
int departureSecondsCounter = 999999;
int weatherSecondsCounter = 999999;
int ScrollingTextSecondsCounter = 999999;

void loop() {

  unsigned long now = millis();

  // Sekunden hochzählen
  if (now - lastUpdate > 1000) {
    lastUpdate = now;

    departureSecondsCounter += 1;
    weatherSecondsCounter += 1;
    ScrollingTextSecondsCounter += 1;

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

  if (ScrollingTextSecondsCounter >= 21000) { // alle 6 Stunden
    ScrollingTextSecondsCounter = 0;
    updateScrollingText();
    updateSystemTime(); // Systemzeit alle 6 Stunden aktualisieren, um Drift zu vermeiden
  }
}