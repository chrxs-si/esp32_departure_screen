#ifndef DISPLAY_H
#define DISPLAY_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>

extern MatrixPanel_I2S_DMA *display;

// Panel Konfiguration
#define PANEL_RES_X 64      // Breite des Panels
#define PANEL_RES_Y 32      // Höhe des Panels
#define PANEL_CHAIN 2       // Anzahl der Panels (verkettet)

// Grundfarben
extern uint16_t DEPARTURE_COLOR;
extern uint16_t CLOCK_COLOR;
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

enum VerticalPos {
  ROW_1,
  ROW_2,
  ROW_3,
  ROW_4,
  CENTER,
  CENTER_ABOVE,
  CENTER_BELOW
};

void initDisplay();

VerticalPos getVerticalPosForRow(int row);

int getYFromVerticalPos(VerticalPos pos);

int getTextWidth(const String &text, uint8_t textSize);

void displayText(String text, int line, uint16_t color, TextAlign align = ALIGN_CENTER);

void drawStaticText(const String &text, int xStart = 0, int xEnd = PANEL_RES_X, VerticalPos vPos = ROW_1, TextAlign align = ALIGN_LEFT, uint16_t color = WHITE, uint8_t textSize = 1);

void loadingTransition(int steps = 70, int delayTime = 40);

/*
class ScrollingText {
public:
    ScrollingText(const String &scrollText, int xStartPos, int xEndPos, 
                  VerticalPos verticalPos, uint16_t textColor, 
                  uint8_t textSize, uint8_t scrollSpeedMs, int minGapPixels);
    
    void start();
    void stop();
    void clear();

private:
    static void scrollTask(void *param);
    void prepareText();

    String text;
    int xStart, xEnd, textWidth, loopWidth, offset;
    uint8_t textSize, speedMs;
    uint16_t color;
    VerticalPos vPos;
    int minGap;
    TaskHandle_t taskHandle = nullptr;
};
*/
#endif