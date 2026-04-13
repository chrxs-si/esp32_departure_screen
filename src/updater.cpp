#include "updater.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "wifi_setup.h"
#include "display.h"

// GitHub benötigt zwingend einen User-Agent Header
const char* userAgent = "ESP32-Updater-Client";

String latestVersionLink = ""; // Speicher für den Download-Link des Assets
String latestAssetId = "";
String newestVersion = "";

void updateSystem() {
    newestVersion = checkForSoftwareUpdates();
    loadSettings(); // Einstellungen neu laden, damit die neue Version korrekt angezeigt wird

    if (newestVersion != current_version) {
        String result = updateSoftware();
        Serial.println("[OTA] Update Ergebnis: " + result);
    }
}

String checkForSoftwareUpdates() {
    if (WiFi.status() != WL_CONNECTED) return "";

    display->clearScreen();
    drawStaticText("Suche nach Updates...", 0, PANEL_RES_X * PANEL_CHAIN, CENTER, ALIGN_CENTER, WHITE, 1);
    delay(400);

    WiFiClientSecure client;
    client.setInsecure(); // Zertifikatsprüfung überspringen für einfachere Wartung
    HTTPClient http;

    String url = "https://api.github.com/repos/" + GITHUB_USER + "/" + GITHUB_REPO + "/releases/latest";
    
    Serial.println("[OTA] Prüfe auf Updates: " + url);
    
    http.begin(client, url);
    http.addHeader("Authorization", "token " + GITHUB_TOKEN); // Weil das Repo privat ist, Token übergeben
    http.addHeader("User-Agent", userAgent);
    
    int httpCode = http.GET();
    String newVersion = "";

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            String tag_name = doc["tag_name"].as<String>();
            
            // Suche nach dem ersten Asset, das auf .bin endet
            JsonArray assets = doc["assets"].as<JsonArray>();
            for (JsonObject asset : assets) {
                String fileName = asset["name"].as<String>();
                if (fileName.endsWith(".bin")) {
                    latestVersionLink = asset["browser_download_url"].as<String>();
                    latestAssetId = asset["id"].as<String>();
                    newVersion = tag_name;
                    break;
                }
            }

            if (newVersion != "" && newVersion != CURRENT_VERSION) {
                display->clearScreen();
                drawStaticText("Update verfügbar", 0, PANEL_RES_X * PANEL_CHAIN, CENTER, ALIGN_CENTER, WHITE, 1);
                delay(400);

                Serial.printf("[OTA] Neue Version verfügbar: %s (Aktuell: %s)\n", newVersion.c_str(), CURRENT_VERSION.c_str());
                http.end();
                return newVersion;
            } else {
                display->clearScreen();
                drawStaticText("kein Update", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, WHITE, 1);
                drawStaticText("verfügbar", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, WHITE, 1);
                delay(400);
                Serial.println("[OTA] Software ist auf dem neuesten Stand.");
            }
        } else {
            display->clearScreen();
            drawStaticText("Update Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
            drawStaticText("JSON parsing", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);
            delay(400);
            Serial.println("[OTA] JSON Parsing fehlgeschlagen!");
        }
    } else {
        display->clearScreen();
        drawStaticText("Update Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
        drawStaticText("HTTP Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);
        delay(400);
        Serial.printf("[OTA] HTTP Fehler: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return "";
}

String updateSoftware() {
    if (latestAssetId == "") return "Keine Download-ID gefunden.";

    display->clearScreen();
    drawStaticText("downloade Update...", 0, PANEL_RES_X * PANEL_CHAIN, CENTER, ALIGN_CENTER, WHITE, 1);
    delay(400);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    // GitHub Assets leiten oft auf objects.githubusercontent.com um
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    String url = "https://api.github.com/repos/" + String(GITHUB_USER) + "/" + String(GITHUB_REPO) + "/releases/assets/" + String(latestAssetId);
    Serial.println("[OTA] Starte Download: " + url);
    http.begin(client, url);
    http.addHeader("Authorization", "token " + GITHUB_TOKEN); // Weil das Repo privat ist, Token übergeben
    http.addHeader("User-Agent", userAgent);
    http.addHeader("Accept", "application/octet-stream");

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        display->clearScreen();
        drawStaticText("Download Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
        drawStaticText("HTTP Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);
        delay(400);
        return "Download fehlgeschlagen (HTTP " + String(httpCode) + ")";
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        http.end();
        display->clearScreen();
        drawStaticText("Download Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
        drawStaticText("Filesize error", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);
        delay(400);
        return "Ungültige Dateigröße!";
    }

    // OTA Update Prozess starten
    if (!Update.begin(contentLength)) {
        http.end();
        display->clearScreen();
        drawStaticText("Download Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
        drawStaticText("Memory error", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);
        delay(400);
        return "Nicht genug Platz für das Update!";
    }

    Serial.println("[OTA] Schreibe Flash...");
    display->clearScreen();
    drawStaticText("installiere Update...", 0, PANEL_RES_X * PANEL_CHAIN, CENTER, ALIGN_CENTER, WHITE, 1);
    delay(400);

    WiFiClient* stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);

    if (written != contentLength) {
        http.end();
        display->clearScreen();
        drawStaticText("Installations Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
        drawStaticText("writing error", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);
        delay(400);
        return "Schreibfehler: " + String(written) + "/" + String(contentLength) + " Bytes geschrieben.";
    }

    if (!Update.end()) {
        http.end();
        display->clearScreen();
        drawStaticText("Installations Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
        drawStaticText("end error - 1", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);
        delay(400);
        return "Update.end() fehlgeschlagen! Fehler: " + String(Update.getError());
    }

    if (!Update.isFinished()) {
        http.end();
        display->clearScreen();
        drawStaticText("Installations Fehler", 0, PANEL_RES_X * PANEL_CHAIN, ROW_2, ALIGN_CENTER, RED, 1);
        drawStaticText("end error - 2", 0, PANEL_RES_X * PANEL_CHAIN, ROW_3, ALIGN_CENTER, RED, 1);
        delay(400);
        return "Update nicht ordnungsgemäß beendet!";
    }

    Serial.println("[OTA] Update erfolgreich abgeschlossen. Starte neu...");
    http.end();

    display->clearScreen();
    drawStaticText("Update installiert!", 0, PANEL_RES_X * PANEL_CHAIN, CENTER, ALIGN_CENTER, GREEN, 1);
    delay(1000);
    display->clearScreen();
    drawStaticText("Neustart!", 0, PANEL_RES_X * PANEL_CHAIN, CENTER, ALIGN_CENTER, GREEN, 1);

    current_version = newestVersion; // Lokale Version aktualisieren, damit die Anzeige korrekt ist
    saveSettings(); // Neue Version speichern, damit sie nach dem Neustart korrekt angezeigt wird
    
    // Wir geben die Info zurück, aber der ESP sollte idealerweise neu starten
    delay(1000);
    ESP.restart(); 
    
    return "Erfolg"; 
}