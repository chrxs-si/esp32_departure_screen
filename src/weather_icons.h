#pragma once
#include "display.h"


void drawSun(int sunX = PANEL_RES_X - 6, int sunY = 4, int sunRadius = 3, uint16_t sunColor = YELLOW);

void drawCloud(int cloudX = PANEL_RES_X - 10, int cloudY = 0, uint16_t cloudColor = WHITE);

void startRainTask(int intensity = 3, int delayMs = 150, int length = 2, uint16_t color = BLUE);
void stopRainTask();

void updateWeatherIcon(int weatherCode, bool isDay);