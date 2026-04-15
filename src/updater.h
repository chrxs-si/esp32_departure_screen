#ifndef UPDATER_H
#define UPDATER_H

#include <Arduino.h>

// --- KONFIGURATION ---
const String CURRENT_VERSION = "1.0.0"; // Deine aktuelle lokale Version
const String GITHUB_USER     = "chrxs-si";
const String GITHUB_REPO     = "esp32_departure_screen";
const String GITHUB_TOKEN    = "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";
// ---------------------

void updateSystem();

String checkForSoftwareUpdates();

String updateSoftware();

#endif