#include "clock.h"
#include "display.h"

uint16_t color = BLUE;

// Zeit zeichnen
void drawTime() {
    if (!display) return;

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    char buf[6]; // "HH:MM"
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    String currentTime = String(buf);

    int textWidth = getTextWidth(currentTime, 1); // textSize 1

    drawStaticText(currentTime, 0, 30, ROW_1, ALIGN_LEFT, CLOCK_COLOR, 1);
}