#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>

extern MatrixPanel_I2S_DMA *display;

// Panel Konfiguration
#define PANEL_RES_X 64      // Breite des Panels
#define PANEL_RES_Y 32      // Höhe des Panels
#define PANEL_CHAIN 1       // Anzahl der Panels (verkettet)

// Grundfarben
extern uint16_t DEPARTURE_COLOR;
extern uint16_t BLACK;
extern uint16_t WHITE;
extern uint16_t RED;
extern uint16_t GREEN;
extern uint16_t BLUE;
extern uint16_t YELLOW;
extern uint16_t CYAN;
extern uint16_t MAGENTA;
extern uint16_t ORANGE;
extern uint16_t PURPLE;
extern uint16_t PINK;
extern uint16_t BROWN;
extern uint16_t GRAY;
extern uint16_t LIGHTGRAY;
extern uint16_t DARKGREEN;
extern uint16_t LIGHTBLUE;

enum TextAlign {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
};

void initDisplay();

void Intro();

void displayText(String text, int line, uint16_t color, TextAlign align = ALIGN_CENTER);

void loadingTransition(int steps = 70, int delayTime = 40);