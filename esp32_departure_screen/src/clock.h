#pragma once
#include "display.h"

class Clock {
public:
    // Konstruktor
    Clock(TextAlign align = ALIGN_LEFT, int row = 0, uint16_t color = WHITE);

    // Methoden
    void start();      // Uhr starten
    void stop();       // Uhr stoppen
    void clear();      // Bereich der Uhr auf BLACK setzen

private:
    TextAlign align;
    int row;
    VerticalPos vPos;
    uint16_t color;
    String currentTime;

    TaskHandle_t taskHandle;

    // Task-Funktion
    static void clockTask(void *param);

    // interne Methode zum Zeichnen der Uhr
    void drawTime();
};