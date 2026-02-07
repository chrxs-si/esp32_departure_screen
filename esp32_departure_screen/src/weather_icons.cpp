#include "weather_icons.h"
#include "display.h"

int rainIntensity = 0; // 0 = kein Regen, 1 = leichter Regen, 2 = mäßiger Regen, 3 = starker Regen
int rainDelayMs = 150;
uint16_t rainColor = BLUE;

void drawCloud(int cloudX, int cloudY, uint16_t cloudColor) {
    // Wolke zeichnen (kompakte Form, max 4 Pixel hoch)
    display->fillCircle(cloudX + 1, cloudY + 2, 1, cloudColor);
    display->fillCircle(cloudX + 4, cloudY + 1, 1, cloudColor);
    display->fillCircle(cloudX + 7, cloudY + 2, 1, cloudColor);
    display->fillRect(cloudX + 1, cloudY + 2, 6, 2, cloudColor);
}

struct RainParams {
  int intensity;
  int delayTicks;
  int X;
  int Y;
};

// Task für animierte Regenwolke rechts oben
void rainTask(void *pvParameters) {
  int rainFrame = 0;          
  const TickType_t delayTicks = rainDelayMs / portTICK_PERIOD_MS;

  const int rainStartX = PANEL_RES_X - 4;
  const int rainStartY = 4;   // Regen innerhalb der 8 Pixel

  while (true) {
    if (!display) {
      vTaskDelay(delayTicks);
      continue;
    }

    // Vorherige Regenstreifen löschen
    for (int i = 0; i < rainIntensity; i++) {
      int x = rainStartX + i * 3 - 2 * (rainIntensity - 1);
      display->drawLine(x, rainStartY, x, rainStartY + 3, BLACK);
    }

    // Aktuelle Regenstreifen zeichnen
    for (int i = 0; i < rainIntensity; i++) {
      int offset = (rainFrame + i) % 3; // maximal 3 Pixel hoch
      int x = rainStartX + i * 3 - 2 * (rainIntensity - 1);
      display->drawLine(x, rainStartY + offset, x, rainStartY + 1 + offset, rainColor);
    }

    rainFrame = (rainFrame + 1) % 3;

    vTaskDelay(delayTicks);
  }
}

void startRainTask(int intensity, int delayMs, uint16_t color) {
  rainIntensity = intensity;
  rainColor = color;
  rainDelayMs = delayMs;

  xTaskCreate(
    rainTask,      // Task-Funktion
    "Rain",        // Name
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
    drawCloud(cloudX, cloudY, cloudColor);
}