#include "display.h"

uint16_t DEPARTURE_COLOR;
uint16_t BLACK;
uint16_t WHITE;
uint16_t RED;
uint16_t GREEN;
uint16_t BLUE;
uint16_t YELLOW;
uint16_t CYAN;
uint16_t MAGENTA;
uint16_t ORANGE;
uint16_t PURPLE;
uint16_t PINK;
uint16_t BROWN;
uint16_t GRAY;
uint16_t LIGHTGRAY;
uint16_t DARKGREEN;
uint16_t LIGHTBLUE;

MatrixPanel_I2S_DMA *display = nullptr;

void initDisplay() {

  DEPARTURE_COLOR  = display->color565(255, 165, 0);
  BLACK   = display->color565(0, 0, 0);
  WHITE   = display->color565(255, 255, 255);
  RED     = display->color565(255, 0, 0);
  GREEN   = display->color565(0, 255, 0);
  BLUE    = display->color565(0, 0, 255);
  YELLOW  = display->color565(255, 255, 0);
  CYAN    = display->color565(0, 255, 255);
  MAGENTA = display->color565(255, 0, 255);
  ORANGE  = display->color565(255, 165, 0);
  PURPLE  = display->color565(128, 0, 128);
  PINK    = display->color565(255, 192, 203);
  BROWN   = display->color565(165, 42, 42);
  GRAY    = display->color565(128, 128, 128);
  LIGHTGRAY = display->color565(192, 192, 192);
  DARKGREEN = display->color565(0, 100, 0);
  LIGHTBLUE = display->color565(173, 216, 230);
}

void Intro() {
  const char* greetings[] = { "Hallo!", "Hello!", "Bonjour!","Shalom!","Salam!","Hej!","Hei!","Ciao!","hola!","MOIN!"
  };
  for (int i = 0; i < sizeof(greetings) / sizeof(greetings[0]); i++) {
    display->fillScreen(BLACK);
    displayText(greetings[i], -1, WHITE);
    delay(600);
  }
  delay(1000);
  displayText("MOIN!", -1, RED);
  delay(1000);
  display->fillScreen(BLACK);
  delay(500);
  displayText("Digitale", 0, RED);
  delay(500);
  displayText("Abfahrts-", 1, GREEN);
  delay(500);
  displayText("Anzeige", 2, BLUE);
  delay(500);
  displayText("by Chrissi", 3, YELLOW);
  delay(4000);
}


String normalizeUmlauts(String text) {
  text.replace("ä", "ae");
  text.replace("ö", "oe");
  text.replace("ü", "ue");
  text.replace("Ä", "Ae");
  text.replace("Ö", "Oe");
  text.replace("Ü", "Ue");
  text.replace("ß", "ss");
  return text;
}

// Funktion zum Zeichnen von Text mit optionaler horizontaler Ausrichtung
void displayText(String text, int line, uint16_t color, TextAlign align) {
  int16_t x1, y1;
  uint16_t w, h;

  display->setTextSize(1);
  display->setTextColor(color);

  text = normalizeUmlauts(text);

  // Textbox berechnen
  display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int x = 0;
  int y = 0;

  // Vertikale Position
  if (line == -1) {
    // Komplett zentriert
    y = (PANEL_RES_Y - h) / 2;
  } else if (line >= 0 && line <= 3) {
    int lineHeight = PANEL_RES_Y / 4;
    y = line * lineHeight + (lineHeight - h) / 2;
  }

  // Horizontale Ausrichtung
  switch (align) {
    case ALIGN_LEFT:
      x = 0;
      break;

    case ALIGN_CENTER:
      x = (PANEL_RES_X - w) / 2;
      break;

    case ALIGN_RIGHT:
      x = PANEL_RES_X - w;
      break;
  }

  display->setCursor(x, y);
  display->print(text);
}


void loadingTransition(int steps, int delayTime) {
  // Schritt 1: Zufällige bunte Rechtecke
  for (int i = 0; i < steps; i++) {
    int rectWidth  = random(5, 20);  // zufällige Breite
    int rectHeight = random(5, 20);  // zufällige Höhe
    int x = random(0, PANEL_RES_X - rectWidth);
    int y = random(0, PANEL_RES_Y - rectHeight);

    // Zufällige Farbe
    uint8_t r = random(0, 256);
    uint8_t g = random(0, 256);
    uint8_t b = random(0, 256);
    uint16_t color = display->color565(r, g, b);

    display->fillRect(x, y, rectWidth, rectHeight, color);
    delay(delayTime); // Geschwindigkeit des Effekts
  }

  // Schritt 2: Schwarzes Rechteck aus der Mitte
  int maxWidth = PANEL_RES_X;
  int maxHeight = PANEL_RES_Y;
  int centerX = PANEL_RES_X / 2;
  int centerY = PANEL_RES_Y / 2;

  for (int i = 0; i <= max(maxWidth, maxHeight); i += 2) {
    int x = centerX - i / 2;
    int y = centerY - i / 2;
    int w = i;
    int h = i;

    // Begrenzung auf Panelgröße
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > PANEL_RES_X) w = PANEL_RES_X - x;
    if (y + h > PANEL_RES_Y) h = PANEL_RES_Y - y;

    display->fillRect(x, y, w, h, BLACK);
    delay(15); // Geschwindigkeit des "Schwarzes Rechteck wächst" Effekts
  }
}