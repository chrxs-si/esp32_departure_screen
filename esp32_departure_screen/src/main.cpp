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

void updateWeather() {
  Serial.print("update Weather.");

  String weatherJson = fetchWeatherJson(52.5170365, 13.3888599);
  WeatherData data = parseWeatherJson(weatherJson, data) ? data : WeatherData();

  updateTemprature((int)round(data.temperature_2m));
  updateWeatherIcon(data.weather_code);
}

void updateScrollingText() {
  Serial.print("update scrolling Text.");

}

void setup() {
  Serial.begin(115200);
  Serial.println("starte setup...");
  initDisplay();

  randomSeed(esp_random()); 

  //loadingTransition(70);
  if (false) {
    selectedStopID = "900024208";
    selectedLine = "101";

    connectToWifi(selectedSSID, selectedPassword);
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
  
  // Set systemtime and timezone to CET/CEST
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now;
  while (time(&now) < 100000) {
    delay(100);
  }
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();

  updateWeather();

  //line1text = new ScrollingText("Hallo Kai!! Es ist bald 15 Uhr!", 0, PANEL_RES_X, ROW_4, PURPLE, 1, 80, 8);
  //line1text->start();

  myClock.start();
}

int lastUpdate = millis();
int departureSecondsCounter = 0;
int weatherSecondsCounter = 0;
int ScrollingTextSecondsCounter = 0;

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
    updateWeather();
  }

  if (ScrollingTextSecondsCounter >= 21000) { // alle 6 Stunden
    ScrollingTextSecondsCounter = 0;
    updateScrollingText();
  }
}