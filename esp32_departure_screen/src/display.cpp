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
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);

  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(100);

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


int getYFromVerticalPos(VerticalPos pos) {
  const int rowHeight = 8; // Standard 6x8 Font
  switch (pos) {
    case ROW_1: return 0;
    case ROW_2: return rowHeight;
    case ROW_3: return rowHeight * 2;
    case ROW_4: return rowHeight * 3;
    case CENTER: return (PANEL_RES_Y / 2) - (rowHeight / 2);
    case CENTER_ABOVE: return (PANEL_RES_Y / 2) - rowHeight;
    case CENTER_BELOW: return (PANEL_RES_Y / 2);
  }
  return 0;
}

int getTextWidth(const String &text, uint8_t textSize) {
  int16_t x1, y1;
  uint16_t w, h;
  display->setTextSize(textSize);
  display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

void drawStaticText(const String &text, int xStart, int xEnd, VerticalPos vPos, TextAlign align, uint16_t color, uint8_t textSize) {
  display->setTextSize(textSize);
  display->setTextColor(color);

  String normalizedText = normalizeUmlauts(text);

  int y = getYFromVerticalPos(vPos);
  int textWidth = getTextWidth(normalizedText, textSize);

  int x;
  switch (align) {
    case ALIGN_LEFT:
      x = xStart;
      break;
    case ALIGN_CENTER:
      x = xStart + ((xEnd - xStart - textWidth) / 2);
      break;
    case ALIGN_RIGHT:
      x = xEnd - textWidth;
      break;
  }

  display->setCursor(x, y);
  display->print(normalizedText);
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


// Konstruktor
ScrollingText::ScrollingText(
    const String &t,
    int xs, int xe,
    VerticalPos vp,
    uint16_t c,
    uint8_t ts,
    uint8_t sp,
    int gap
) {
    text = t;
    xStart = xs;
    xEnd = xe;
    vPos = vp;
    color = c;
    textSize = ts;
    speedMs = sp;
    minGap = gap;

    offset = 0;
    lastUpdate = millis();

    prepareText();
}

// Text vorbereiten: Leerzeichen anhängen falls nötig
void ScrollingText::prepareText() {
    int textWidth = getTextWidth(text, textSize);
    int areaWidth = xEnd - xStart;

    if (textWidth < areaWidth) {
        int spaceWidth = getTextWidth(" ", textSize);
        int spacesToAdd = ((areaWidth + minGap - textWidth) / spaceWidth) + 1;
        for (int i = 0; i < spacesToAdd; i++) text += " ";
        textWidth = getTextWidth(text, textSize);
    }

    loopWidth = textWidth + minGap;
}

// Update-Methode, in loop() aufrufen
void ScrollingText::update() {
    unsigned long now = millis();
    if (now - lastUpdate < speedMs) return;
    lastUpdate = now;

    offset += 1;
    if (offset >= loopWidth) offset -= loopWidth;

    int y = getYFromVerticalPos(vPos);
    int areaWidth = xEnd - xStart;

    // Bereich löschen
    display->fillRect(xStart, y, areaWidth, 8 * textSize, BLACK);

    display->setTextSize(textSize);
    display->setTextColor(color);

    // Erste Instanz
    int x1 = xEnd - offset;
    display->setCursor(x1, y);
    display->print(text);

    // Zweite Instanz direkt dahinter für nahtlosen Übergang
    int x2 = x1 + loopWidth;
    display->setCursor(x2, y);
    display->print(text);
}