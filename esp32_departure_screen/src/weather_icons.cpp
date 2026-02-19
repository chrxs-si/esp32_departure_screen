#include "weather_icons.h"
#include "display.h"

int rainIntensity = 0; // 0 = kein Regen, 1 = leichter Regen, 2 = mäßiger Regen, 3 = starker Regen
int rainDelayMs = 150;
int rainLength = 2; // Länge der Regenstreifen in Pixeln, 1 oder 2
uint16_t rainColor = BLUE;

TaskHandle_t rainTaskHandle = NULL;

void drawCloud(int cloudX, int cloudY, uint16_t cloudColor) {
    // Wolke zeichnen (kompakte Form, max 4 Pixel hoch)
    display->fillCircle(cloudX + 1, cloudY + 2, 1, cloudColor);
    display->fillCircle(cloudX + 4, cloudY + 1, 1, cloudColor);
    display->fillCircle(cloudX + 7, cloudY + 2, 1, cloudColor);
    display->fillRect(cloudX + 1, cloudY + 2, 6, 2, cloudColor);
}

void drawSun(int sunX, int sunY, int sunRadius, uint16_t sunColor) {
    // Sonne
    display->fillCircle(sunX, sunY, sunRadius, sunColor);

}

// Task für animierte Regenwolke rechts oben
void rainTask(void *pvParameters) {
  int rainFrame = 0;          
  const TickType_t delayTicks = rainDelayMs / portTICK_PERIOD_MS;

  const int centerX = PANEL_RES_X - 5; // echte Mitte der Regenstreifen
  const int rainStartY = 4;

  const int spacing = 2;

  while (true) {
    if (!display) {
      vTaskDelay(delayTicks);
      continue;
    }

    // Alte Regenstreifen löschen
    for (int i = 0; i < rainIntensity; i++) {
      int x = centerX + (i - (rainIntensity - 1) / 1.5f) * spacing;
      display->drawLine(x, rainStartY, x, rainStartY + rainLength + 2, BLACK);
    }

    // Neue Regenstreifen zeichnen
    for (int i = 0; i < rainIntensity; i++) {
      int offset = (rainFrame + i) % 3;
      int x = centerX + (i - (rainIntensity - 1) / 1.5f) * spacing;
      display->drawLine(x, rainStartY + offset, x, rainStartY + rainLength - 1 + offset, rainColor);
    }

    rainFrame = (rainFrame + 1) % 3;

    vTaskDelay(delayTicks);
  }
}

void startRainTask(int intensity, int delayMs, int length, uint16_t color) {
  rainIntensity = intensity;
  rainColor = color;
  rainDelayMs = delayMs;
  rainLength = length;

  if (rainTaskHandle != NULL) {
    vTaskDelete(rainTaskHandle);
    rainTaskHandle = NULL;
  }

  xTaskCreate(
    rainTask,      // Task-Funktion
    "Rain",        // Name
    2048,               // Stackgröße
    NULL,               // Parameter
    1,                  // Priorität
    &rainTaskHandle     // Task-Handle
  );
}

void stopRainTask() {
  if (rainTaskHandle != NULL) {
    vTaskDelete(rainTaskHandle);
    rainTaskHandle = NULL;
  }

  // Regenbereich löschen
  display->fillRect(PANEL_RES_X - 12, 0, 12, 8, BLACK);
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



void updateWeatherIcon(int weatherCode, bool isDay) {
  stopRainTask();

  display->fillRect(PANEL_RES_X - 12, 0, 12, 8, BLACK);

  weatherCode = 2;

  switch (weatherCode) {
    case 0: // Klar
      drawSun(PANEL_RES_X - 6, 4, 3, YELLOW);
      if (!isDay) { drawSun(PANEL_RES_X - 8, 4, 2, BLACK); }
      break;
    case 1: // kaum bewölkt
      drawSun(PANEL_RES_X - 3, 3, 2, YELLOW);
      if (!isDay) { drawSun(PANEL_RES_X - 5, 3, 2, BLACK); }
      drawCloud(PANEL_RES_X - 10, 3, WHITE);
      break;
    case 2: // mittel Bewölkt
      drawSun(PANEL_RES_X - 3, 2, 1, YELLOW);
      if (!isDay) { drawSun(PANEL_RES_X - 5, 2, 1, BLACK); }
      drawCloud(PANEL_RES_X - 10, 2, WHITE);
      break;
    case 3: // Bewölkt
    case 45: // fog
    case 48: // Fog and depositing rime fog
      drawCloud(PANEL_RES_X - 10, 2, WHITE);
      break;

    case 51: // leichter Nieselregen
    case 56: // leicher eiskalter Nieselregen
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(1, 180, 1, BLUE);
      break;
    case 53: // mittlerer Nieselregen
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(2, 150, 1, BLUE);
      break;
    case 55: // starker Nieselregen
    case 57: // starker eiskalter Nieselregen
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(3, 120, 1, BLUE);
      break;

    case 61: // leichter Regen
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(2, 180, 2, BLUE);
      break;
    case 63: // mittlerer Regen
    case 66: // light freezing Rain
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(3, 150, 2, BLUE);
      break;
    case 65: // starker Regen
    case 67: // heavy freezing Rain
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(4, 120, 2, BLUE);
      break;

    case 71: // leichter Schnee
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(2, 550, 1, LIGHTBLUE);
      break;
    case 73: // mittlerer Schnee
    case 77: // Schneekörner
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(3, 500, 1, LIGHTBLUE);
      break;
    case 75: // starker Schnee
      drawCloud(PANEL_RES_X - 10, 0, WHITE);
      startRainTask(4, 450, 1, LIGHTBLUE);
      break;

    case 80: // slight rain shower
      drawCloud(PANEL_RES_X - 10, 0, GRAY);
      startRainTask(2, 180, 2, BLUE);
      //Blitz
      break;
    case 81: // moderate rain shower
    case 95: // Thunderstorm
      drawCloud(PANEL_RES_X - 10, 0, GRAY);
      startRainTask(3, 150, 2, BLUE);
      //Blitz
      break;
    case 82: // violent rain shower
      drawCloud(PANEL_RES_X - 10, 0, GRAY);
      startRainTask(4, 120, 2, BLUE);
      //Blitz
      break;

    case 85: // slight snow shower
    case 96: // Thunderstorm with slight hail
      drawCloud(PANEL_RES_X - 10, 0, GRAY);
      startRainTask(3, 250, 1, LIGHTBLUE);
      //Blitz
      break;
    case 86: // heavy snow shower
    case 99: // Thunderstorm with slight hail
      drawCloud(PANEL_RES_X - 10, 0, GRAY);
      startRainTask(4, 200, 1, LIGHTBLUE);
      //Blitz
      break;

    default:
      // Standard-Symbol oder nichts anzeigen
      break;
  }

}