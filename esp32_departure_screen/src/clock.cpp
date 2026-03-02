#include "clock.h"
#include "display.h"

uint16_t color = BLUE;

Clock::Clock(TextAlign align, int row)
    : align(align), row(row), taskHandle(nullptr)
{
    vPos = getVerticalPosForRow(row);
}

// Uhr starten
void Clock::start() {
    if (taskHandle == nullptr) {
        xTaskCreate(
            Clock::clockTask,
            "ClockTask",
            2048,       // Stack-Größe
            this,       // Parameter
            1,          // Priorität
            &taskHandle
        );
    }
}

// Uhr stoppen
void Clock::stop() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
        clear();
    }
}

// Uhrbereich löschen
void Clock::clear() {
    if (!display) return;

    // Text positionieren berechnen
    int y = getYFromVerticalPos(vPos);
    int textWidth = getTextWidth(currentTime, 1); // textSize 1

    int x = 0;
    switch (align) {
        case ALIGN_LEFT: x = 0; break;
        case ALIGN_CENTER: x = (PANEL_RES_X - textWidth) / 2; break;
        case ALIGN_RIGHT: x = PANEL_RES_X - textWidth; break;
    }

    int rowHeight = 8; // Standardhöhe
    display->fillRect(x, y, textWidth, rowHeight, BLACK);
}

// Task-Funktion
void Clock::clockTask(void *param) {
    Clock *self = static_cast<Clock*>(param);

    while (true) {
        self->drawTime();
        vTaskDelay(pdMS_TO_TICKS(3000)); // jede 3 Sekunden aktualisieren
    }
}

// Zeit zeichnen
void Clock::drawTime() {
    if (!display) return;

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[6]; // "HH:MM"
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    currentTime = String(buf);

    int textWidth = getTextWidth(currentTime, 1); // textSize 1

    //clear(); // vorherigen Bereich löschen

    int x = 0;
    switch (align) {
        case ALIGN_LEFT: x = 0; break;
        case ALIGN_CENTER: x = (PANEL_RES_X - textWidth) / 2; break;
        case ALIGN_RIGHT: x = PANEL_RES_X - textWidth; break;
    }

    //drawStaticText(currentTime, x, x + textWidth, vPos, align, LIGHTBLUE, 1);
    drawStaticText(currentTime, 0, 30, ROW_1, ALIGN_LEFT, BLUE, 1);
}