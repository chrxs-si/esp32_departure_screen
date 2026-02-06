#include "weather_icons.h"
#include "display.h"

void cloud(int cloudX = PANEL_RES_X - 10, int cloudY = 0, uint16_t cloudColor = WHITE) {
    // Wolke zeichnen (kompakte Form, max 4 Pixel hoch)
    display->fillCircle(cloudX + 1, cloudY + 2, 1, cloudColor);
    display->fillCircle(cloudX + 4, cloudY + 1, 1, cloudColor);
    display->fillCircle(cloudX + 7, cloudY + 2, 1, cloudColor);
    display->fillRect(cloudX + 1, cloudY + 2, 6, 2, cloudColor);
}

// Task für animierte Regenwolke rechts oben
void rainCloudTask(void *pvParameters) {
  int rainFrame = 0;          
  const TickType_t delayTicks = 150 / portTICK_PERIOD_MS;

  const uint16_t cloudColor = WHITE;
  const uint16_t rainColor  = BLUE;
  const uint16_t bgColor    = BLACK;

  const int cloudX = PANEL_RES_X - 10; // rechts oben
  const int cloudY = 0;                // ganz oben
  const int rainStartX = cloudX + 2;
  const int rainStartY = cloudY + 4;   // Regen innerhalb der 8 Pixel

  while (true) {
    if (!display) {
      vTaskDelay(delayTicks);
      continue;
    }

    cloud(cloudX, cloudY, cloudColor);

    // Vorherige Regenstreifen löschen
    for (int i = 0; i < 3; i++) {
      display->drawLine(rainStartX + i * 3, rainStartY, rainStartX + i * 3, rainStartY + 3, bgColor);
    }

    // Aktuelle Regenstreifen zeichnen
    for (int i = 0; i < 3; i++) {
      int offset = (rainFrame + i) % 3; // maximal 3 Pixel hoch
      display->drawLine(rainStartX + i * 3, rainStartY + offset, rainStartX + i * 3, rainStartY + 1 + offset, rainColor);
    }

    rainFrame = (rainFrame + 1) % 3;

    vTaskDelay(delayTicks);
  }
}

void startRainCloudTask() {
  xTaskCreate(
    rainCloudTask,      // Task-Funktion
    "RainCloud",        // Name
    2048,               // Stackgröße
    NULL,               // Parameter
    1,                  // Priorität
    NULL                // Task-Handle
  );
}

// Kleine Wolke rechts oben mit Sonne dahinter, max. 8 Pixel hoch
void cloudWithSun(int cloudX, int cloudY, uint16_t cloudColor, uint16_t sunColor) {
    // Sonne hinter der Wolke, etwas höher
    int sunRadius = 3;
    int sunX = cloudX + 5;  // rechts hinter der Wolke
    int sunY = cloudY - 2;  // etwas höher, maximal 8 Pixel hoch

    // Sonne als Kreis
    display->fillCircle(sunX, sunY, sunRadius, sunColor);

    // Wolke zeichnen (nutzt vorhandene Funktion)
    cloud(cloudX, cloudY, cloudColor);
}