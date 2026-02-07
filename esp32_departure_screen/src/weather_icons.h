#include "display.h"


void drawCloud(int cloudX = PANEL_RES_X - 10, int cloudY = 0, uint16_t cloudColor = WHITE);

void startRainTask(int intensity = 3, int delayMs = 150, uint16_t color = BLUE);