#ifndef UPDATER_H
#define UPDATER_H

#include <Arduino.h>

// --- KONFIGURATION ---
const String CURRENT_VERSION = "1.0.0"; // Deine aktuelle lokale Version
const String GITHUB_USER     = "chrxs-si";
const String GITHUB_REPO     = "esp32_departure_screen";

// Falls GITHUB_TOKEN_VAL nicht über Build-Flags definiert wurde, Fallback auf leer
#ifndef GITHUB_TOKEN_VAL
  #define GITHUB_TOKEN_VAL "placeholder"
#endif

const String GITHUB_TOKEN = GITHUB_TOKEN_VAL;
// ---------------------

void updateSystem();

#endif