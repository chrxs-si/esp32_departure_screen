#include "display.h"
#include <time.h>

uint16_t DEPARTURE_COLOR;
uint16_t CLOCK_COLOR;
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
  /*
  // RGB Pins
  #define R1_PIN 25
  #define G1_PIN 26
  #define B1_PIN 27
  #define R2_PIN 14
  #define G2_PIN 12
  #define B2_PIN 13

  #define A_PIN 23
  #define B_PIN 19
  #define C_PIN 5
  #define D_PIN 17
  #define E_PIN -1

  // Steuerpins
  #define LAT_PIN 4
  #define OE_PIN 15
  #define CLK_PIN 16

  #define R1_PIN 25
  #define G1_PIN 26
  #define B1_PIN 27
  #define R2_PIN 14
  #define G2_PIN 13  // Verschoben von 12 (Strapping Pin)
  #define B2_PIN 32  // Neuer sicherer Pin

  // Adressleitungen
  #define A_PIN 23
  #define B_PIN 19
  #define C_PIN 22  // Verschoben von 5 (Strapping Pin)
  #define D_PIN 21
  #define E_PIN -1  // Nur bei 1/32 Panels nötig

  // Steuerpins
  #define LAT_PIN 4
  #define OE_PIN  33  // Verschoben von 15 (Strapping Pin)
  #define CLK_PIN 18  // Sicherer Takt-Pin

  */
  // --- High-Speed Datenleitungen (Saubere Pins) ---
  // 4er Kabel: R1, G1, B1, R2
  #define R1_PIN 25
  #define B1_PIN 26
  #define R2_PIN 27
  #define B2_PIN 14

  // Einzelne Kabel: G1, G2
  #define G1_PIN 21
  #define G2_PIN 13

  // --- Langsame Adress- und Steuerleitungen ---
  // 2er Kabel: A, D
  #define A_PIN 32
  #define C_PIN 33

  // 4er Kabel: B, D, LAT, GND
  #define B_PIN 5
  #define D_PIN 18
  #define LAT_PIN 19 // eigentlich auch ein Kotroll-Pin, aber hier mit 4er Kabel angebunden
  #define E_PIN -1

  // --- Kontroll-Pins ---
  // 2er Kabel: OE, CLK
  #define CLK_PIN 22
  #define OE_PIN 23

  // Pinstruktur der Library
  HUB75_I2S_CFG::i2s_pins pins = {
      R1_PIN, G1_PIN, B1_PIN,
      R2_PIN, G2_PIN, B2_PIN,
      A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
      LAT_PIN, OE_PIN, CLK_PIN
  };

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN, pins);

  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG; 
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  mxconfig.clkphase = false;
  
  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();
  display->setBrightness8(100);

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
  DARKGREEN = display->color565(0, 255, 0);
  LIGHTBLUE = display->color565(173, 216, 230);

  DEPARTURE_COLOR  = ORANGE;
  CLOCK_COLOR  = BLUE;
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

VerticalPos getVerticalPosForRow(int row) {
  switch (row) {
    case -1: return CENTER_ABOVE;
    case -2: return CENTER;
    case -3: return CENTER_BELOW;
    case 0: return ROW_1;
    case 1: return ROW_2;
    case 2: return ROW_3;
    case 3: return ROW_4;
    default: return ROW_1; // Fallback
  }
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
    if (!display) return 0;

    display->setTextSize(textSize);
    display->setTextWrap(false);

    int16_t x = 0;
    int16_t y = 0;
    int16_t x1, y1;
    uint16_t w, h;

    for (uint16_t i = 0; i < text.length(); i++) {
        char c = text[i];
        display->getTextBounds(String(c), x, y, &x1, &y1, &w, &h);
        x += w;  // Cursor-Advance simulieren
    }

    return x;
}

void drawStaticText(const String &text, int xStart, int xEnd, VerticalPos vPos, TextAlign align, uint16_t color, uint8_t textSize) {
  if (!display) return;

  display->setTextSize(textSize);
  display->setTextColor(color);

  String normalizedText = normalizeUmlauts(text);

  int y = getYFromVerticalPos(vPos);
  int textWidth = getTextWidth(normalizedText, textSize);

  int areaWidth = xEnd - xStart;
  int fontHeight = 8 * textSize;
  display->fillRect(xStart, y, areaWidth, fontHeight, BLACK);

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

/*
// Konstruktor
ScrollingText::ScrollingText(
    const String &scrollText,
    int xStartPos,
    int xEndPos,
    VerticalPos verticalPos,
    uint16_t textColor,
    uint8_t textSize,
    uint8_t scrollSpeedMs,
    int minGapPixels
) {
    text = normalizeUmlauts(scrollText);
    xStart = xStartPos;
    xEnd = xEndPos;
    vPos = verticalPos;
    color = textColor;
    this->textSize = textSize;
    speedMs = scrollSpeedMs;
    minGap = minGapPixels;

    offset = 0;
    prepareText();
}

// Text vorbereiten
void ScrollingText::prepareText() {
    if (!display) return;

    int areaWidth = xEnd - xStart;
    textWidth = getTextWidth(text, textSize);

    if (textWidth <= areaWidth) {
        int spaceWidth = getTextWidth(" ", textSize);
        int requiredWidth = areaWidth + minGap;

        int spacesToAdd = ((requiredWidth - textWidth) / spaceWidth) + 1;
        for (int i = 0; i < spacesToAdd; i++) {
            text += ' ';
        }

        textWidth = getTextWidth(text, textSize);
    }

    loopWidth = textWidth + minGap;
}

// Startet den Scrolltext asynchron
void ScrollingText::start() {
    if (taskHandle != nullptr) return; // Task läuft bereits

    xTaskCreate(
        scrollTask,
        "ScrollTask",
        2048,
        this,
        1,
        &taskHandle
    );
}

// Stoppt den laufenden Task
void ScrollingText::stop() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}

// Löscht den Text auf dem Display
void ScrollingText::clear() {
    stop(); // Sicherstellen, dass der Task gestoppt ist
    if (!display) return;
    int areaWidth = xEnd - xStart;
    int fontHeight = 8 * textSize;
    int y = getYFromVerticalPos(vPos);

    display->fillRect(xStart, y, areaWidth, fontHeight + 1, BLACK);
}

// FreeRTOS Task-Funktion
void ScrollingText::scrollTask(void *param) {
    ScrollingText *self = static_cast<ScrollingText*>(param);

    int areaWidth = self->xEnd - self->xStart;
    // WICHTIG: Erhöhe die fontHeight leicht, um Ausreißer-Pixel zu löschen
    int fontHeight = 8 * self->textSize; 
    int yPos = getYFromVerticalPos(self->vPos);

    while (true) {
        if (!display) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }

        // 1. Bereich SCHWARZ machen (Löschen)
        // Wir löschen etwas großzügiger (+1 oder +2 Pixel), 
        // um Artefakte in der nächsten Zeile zu vermeiden.
        display->fillRect(self->xStart, yPos, areaWidth, fontHeight, BLACK);

        display->setTextSize(self->textSize);
        display->setTextColor(self->color);
        display->setTextWrap(false);

        // --- CLIPPING LOGIK ---
        // Falls deine Library display->setClipRect(x, y, w, h) unterstützt, hier nutzen!
        // Da viele das nicht haben, begrenzen wir es durch geschicktes Zeichnen:

        int x1 = self->xStart - self->offset;
        
        // Erster Text-Block
        display->setCursor(x1, yPos);
        display->print(self->text);

        // Zweiter Text-Block für den Loop
        if (x1 + self->textWidth < self->xStart + areaWidth) {
            int x2 = x1 + self->loopWidth;
            display->setCursor(x2, yPos);
            display->print(self->text);
        }

        // 2. WICHTIG: "Aufräumen" der Ränder
        // Falls der Text links oder rechts über xStart/xEnd hinausschaut, 
        // übermalen wir die Überstände außerhalb des gewünschten Bereichs.
        // Das verhindert, dass Buchstaben in andere Bereiche "leaken".
        if (self->xStart > 0) {
            display->fillRect(0, yPos, self->xStart, fontHeight, BLACK);
        }
        if (self->xEnd < PANEL_RES_X) {
            display->fillRect(self->xEnd, yPos, PANEL_RES_X - self->xEnd, fontHeight, BLACK);
        }

        self->offset += 1;
        if (self->offset >= self->loopWidth) self->offset -= self->loopWidth;

        vTaskDelay(pdMS_TO_TICKS(self->speedMs));
    }
}

*/