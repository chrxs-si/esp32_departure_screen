#ifndef UPDATER_H
#define UPDATER_H

#include <Arduino.h>

// --- KONFIGURATION ---
const String CURRENT_VERSION = "1.0.0"; // Deine aktuelle lokale Version
const String GITHUB_USER     = "chrxs-si";
const String GITHUB_REPO     = "esp32_departure_screen";
// ---------------------

void updateSystem();

String checkForSoftwareUpdates();

String updateSoftware();

#endif