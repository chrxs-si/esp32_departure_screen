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

void displayText(String text, int line, uint16_t color, TextAlign align = ALIGN_CENTER);

void drawStaticText(const String &text, int xStart = 0, int xEnd = PANEL_RES_X, VerticalPos vPos = ROW_1, TextAlign align = ALIGN_LEFT, uint16_t color = WHITE, uint8_t textSize = 1);

void loadingTransition(int steps = 70, int delayTime = 40);

class ScrollingText {
public:
    // Konstruktor
    ScrollingText(
        const String &t,
        int xs, int xe,
        VerticalPos vp = ROW_1,
        uint16_t c = WHITE,
        uint8_t ts = 1,
        uint8_t sp = 30,
        int gap = 8
    );

    // Muss regelmäßig im loop() aufgerufen werden
    void update();

private:
    String text;
    int xStart;
    int xEnd;
    VerticalPos vPos;
    uint16_t color;
    uint8_t textSize;
    uint8_t speedMs;
    int minGap;

    float offset;
    unsigned long lastUpdate;
    float loopWidth;

    // interne Funktion zur Vorbereitung des Textes (Leerzeichen hinzufügen)
    void prepareText();
};