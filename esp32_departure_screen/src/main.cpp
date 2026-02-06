#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include "time.h"
#include "transport_data.h"
#include "display.h"
#include "weather_icons.h"
#include "wifi_setup.h"

//Wlan & Config-Website --> Find stop id: https://v6.vbb.transport.rest/stops?query=Leibnizstr./B

ScrollingText *line1text;
ScrollingText *line2text;

void setup() {
  Serial.begin(115200);
  Serial.println("starte setup...");
  initDisplay();

  loadingTransition(70);
  if (true) {
    stop1 = "900024208";
    line1 = "101";
    maxColumns1 = 4;
    minOffset1 = 0;

    connectToWiFi(selectedSSID, selectedPassword);
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

  drawStaticText("   Montag", 0, PANEL_RES_X-15, ROW_1, ALIGN_LEFT, BLUE, 1);
  
  startRainCloudTask();

  line1text = new ScrollingText("Hallo Christoph! Schau wie schlau ich bin! Du hottie Karottie!", 0, PANEL_RES_X, ROW_4, PURPLE, 1, 80, 8);
  line1text->start();
}

int lastUpdate = millis();
int speedMs = 30000;

void loop() {
    unsigned long now = millis();
    if (now - lastUpdate > speedMs) {
      server.handleClient();
      updateDepartures();
      lastUpdate = now;
    }
}