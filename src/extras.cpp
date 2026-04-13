#include "extras.h"
#include "display.h"

void animationShapes(int durationMs)
{
  unsigned long start = millis();

  while (millis() - start < durationMs)
  {
    display->clearScreen();

    for (int i = 0; i < 10; i++)
    {
      int x = random(0, PANEL_RES_X * PANEL_CHAIN);
      int y = random(0, PANEL_RES_Y);

      uint16_t colorList[] = {
        RED, GREEN, BLUE, YELLOW,
        CYAN, MAGENTA, ORANGE,
        PURPLE, PINK, LIGHTBLUE
      };

      uint16_t c = colorList[random(0, 10)];

      int r = random(2, 6);

      display->fillCircle(x, y, r, c);
      //display->drawRect(x, y, r + 3, r + 3, c);
    }

    delay(80);
  }
}


// ==============================
// OBEN / UNTEN BALKEN ANIMATION
// ==============================

void animationBars(int offset)
{
  uint16_t colorList[] = {
    RED, GREEN, BLUE, YELLOW,
    CYAN, MAGENTA, ORANGE,
    PURPLE, PINK, LIGHTBLUE
  };

  for (int x = 0; x < PANEL_RES_X * PANEL_CHAIN; x++)
  {
    uint16_t c = colorList[(x + offset) % 10];

    // oben
    display->drawPixel(x, 0, c);
    display->drawPixel(x, 1, c);

    // unten
    display->drawPixel(x, PANEL_RES_Y - 1, c);
    display->drawPixel(x, PANEL_RES_Y - 2, c);
  }
}


// ==============================
// TEXT ANZEIGE
// ==============================

void showWerbungText(const String &top_text, const String &button_text, const int iterations, uint16_t color)
{
  display->clearScreen();

  for (int i = 0; i < iterations; i++)
  {
    display->clearScreen();

    animationBars(i);

    drawStaticText(
      top_text,
      0,
      PANEL_RES_X * PANEL_CHAIN,
      CENTER_ABOVE,
      ALIGN_CENTER,
      color,
      1
    );
    drawStaticText(
      button_text,
      0,
      PANEL_RES_X * PANEL_CHAIN,
      CENTER_BELOW,
      ALIGN_CENTER,
      color,
      1
    );

    delay(40);
  }
}


// ==============================
// HAUPT WERBUNG
// ==============================

void runAd()
{
  // 1. Formen Animation
  animationShapes(4000);

  // 2. WERBUNG
  showWerbungText("kurze Unterbrechung", "- WERBUNG -", 100, RED);

  // 3. Frage
  showWerbungText("Du moechtest auch", "so ein Display?", 120, WHITE);

  // 4. QR Hinweis
  showWerbungText("Scanne den", "QR Code!", 120, WHITE);

  display->clearScreen();
}



// ------------------------------------------------
// Hilfsfunktion Zufallsfarbe
// ------------------------------------------------
uint16_t randomColor() {

  uint8_t r = random(0, 2) ? 255 : random(0, 80);
  uint8_t g = random(0, 2) ? 255 : random(0, 80);
  uint8_t b = random(0, 2) ? 255 : random(0, 80);

  return display->color565(r, g, b);
}

// ------------------------------------------------
// 3. Disco Kreis Explosion
// ------------------------------------------------
void discoCircles(uint32_t durationMs) {
  uint32_t start = millis();

  while (millis() - start < durationMs) {

    display->fillScreen(BLACK);

    for (int i = 0; i < 5; i++) {
      int x = random(PANEL_RES_X * PANEL_CHAIN);
      int y = random(PANEL_RES_Y);
      int r = random(2, 12);

      display->fillCircle(x, y, r, randomColor());
    }

    delay(80);
  }
}

// ------------------------------------------------
// 5. Zufällige Pixel Party
// ------------------------------------------------
void discoPixels(uint32_t durationMs) {

  uint32_t start = millis();

  display->fillScreen(BLACK);

  while (millis() - start < durationMs / 2) {

    int x = random(PANEL_RES_X * PANEL_CHAIN);
    int y = random(PANEL_RES_Y);

    display->drawPixel(x, y, randomColor());

    delay(5);
  }
}

// ======================================================
// 7. Techno Wave Strobe
// ======================================================
void discoTechnoWave(uint32_t durationMs) {

  uint32_t start = millis();

  int offset = 0;

  while (millis() - start < durationMs) {

    for (int x = 0; x < PANEL_RES_X * PANEL_CHAIN; x++) {

      // Sinusartige Wellenbewegung
      int y = 16 + 10 * sin((x + offset) * 0.3);

      // Farbwechsel je nach Position (bunt + dynamisch)
      uint16_t color = display->color565(
        (sin((x + offset) * 0.1) * 127 + 128),
        (sin((x + offset) * 0.15) * 127 + 128),
        (sin((x + offset) * 0.2) * 127 + 128)
      );

      display->drawLine(x, y, x, y, color);
    }

    offset += 4;

    delay(20);
  }
}


// ======================================================
// 8. Farbige Schachbrett Animation
// ======================================================
void discoChecker(uint32_t durationMs) {

  uint32_t start = millis();

  bool flip = false;

  while (millis() - start < durationMs) {

    for (int y = 0; y < 32; y += 4) {
      for (int x = 0; x < PANEL_RES_X * PANEL_CHAIN; x += 4) {

        bool even = ((x + y) / 4) % 2;

        if (even ^ flip)
          display->fillRect(x, y, 4, 4, randomColor());
        else
          display->fillRect(x, y, 4, 4, BLACK);
      }
    }

    flip = !flip;

    delay(90);
  }
}


// ======================================================
// 9. Pulsierender Kreis Mitte
// ======================================================
void discoPulse(uint32_t durationMs) {

  uint32_t start = millis();

  int r = 1;
  int dir = 1;

  while (millis() - start < durationMs) {

    display->fillScreen(BLACK);

    display->fillCircle(PANEL_RES_X / 2, 16, r, randomColor());
    display->fillCircle(PANEL_RES_X * PANEL_CHAIN - PANEL_RES_X / 2, 16, r, randomColor());

    r += dir;

    if (r > 15) dir = -1;
    if (r < 2) dir = 1;

    delay(40);
  }
}


// ======================================================
// 11. Spiral Chaos (rotierende Farbspirale)
// ======================================================
void discoSpiral(uint32_t durationMs) {

  uint32_t start = millis();
  float angleOffset = 0;

  int cx = (PANEL_RES_X * PANEL_CHAIN) / 2;
  int cy = 16;

  while (millis() - start < durationMs) {

    display->fillScreen(BLACK);

    for (int r = 1; r < 20; r++) {
      for (int a = 0; a < 360; a += 20) {

        float rad = (a + angleOffset) * 0.01745;

        int x = cx + cos(rad) * r;
        int y = cy + sin(rad) * r;

        display->drawPixel(x, y, randomColor());
      }
    }

    angleOffset += 10; // Rotation

    delay(30);
  }
}

// ======================================================
// 12. Beat Drop Strobe (harte Blitz-Impulse)
// ======================================================
void discoBeatStrobe(uint32_t durationMs) {

  uint32_t start = millis();

  while (millis() - start < durationMs) {

    // kurzer heller Flash
    display->fillScreen(randomColor());
    delay(30);

    // sofort schwarz (harter Cut)
    display->fillScreen(BLACK);
    delay(random(40, 120));

    // zufällige Streifen als "Drop"
    for (int i = 0; i < 5; i++) {
      int y = random(0, 32);
      display->drawLine(0, y, PANEL_RES_X * PANEL_CHAIN, y, randomColor());
    }

    delay(50);
  }
}

// ======================================================
// 13. Matrix Disco Rain (fallende Neon-Pixel)
// ======================================================
void discoMatrix(uint32_t durationMs) {

  uint32_t start = millis();

  const int width = PANEL_RES_X * PANEL_CHAIN;
  int drops[64];

  // Startpositionen
  for (int i = 0; i < width; i++) {
    drops[i] = random(0, 32);
  }

  while (millis() - start < durationMs) {

    display->fillScreen(BLACK);

    for (int x = 0; x < width; x++) {

      int y = drops[x];

      // Kopf hell
      display->drawPixel(x, y, randomColor());

      // Schweif
      for (int t = 1; t < 5; t++) {
        display->drawPixel(x, y - t, display->color565(0, 50 / t, 0));
      }

      // Bewegung
      drops[x]++;

      if (drops[x] > 32) {
        drops[x] = 0;
      }
    }

    delay(40);
  }
}

// ------------------------------------------------
// RANDOM DISCO START
// ------------------------------------------------
void runDisco(uint32_t durationMs) {

  int r = random(9);

  switch (r) {
    Serial.println("random: " + String(r));
    case 0: discoCircles(durationMs); break;
    case 1: discoPixels(durationMs); break;
    case 2: discoTechnoWave(durationMs); break;
    case 3: discoChecker(durationMs); break;
    case 4: discoPulse(durationMs); break;
    case 5: discoSpiral(durationMs); break;
    case 6: discoBeatStrobe(durationMs); break;
    case 7: discoMatrix(durationMs); break;
  }

  display->clearScreen();
}