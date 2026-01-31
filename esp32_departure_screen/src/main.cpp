#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <time.h>
#include "transport_data.h"
#include "display.h"
#include "wifi_setup.h"

//Wlan & Config-Website --> Find stop id: https://v6.bvg.transport.rest/stops?query=Leibnizstr./B

void setup() {
  Serial.begin(115200);
  Serial.println("starte setup...");

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);

  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(100);
  initDisplay();

  //Intro();
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

  // loop
  while (true) {

    server.handleClient();
    updateDepartures();
    delay(10000);
  }
}



void loop() {

}