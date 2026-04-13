#include "updater.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "wifi_setup.h"

// GitHub benötigt zwingend einen User-Agent Header
const char* userAgent = "ESP32-Updater-Client";

String latestVersionLink = ""; // Speicher für den Download-Link des Assets

void updateSystem() {
    String newestVersion = checkForSoftwareUpdates();
    if (newestVersion != current_version) {
        String result = updateSoftware();
        Serial.println("[OTA] Update Ergebnis: " + result);
    }
}

String checkForSoftwareUpdates() {
    if (WiFi.status() != WL_CONNECTED) return "";

    WiFiClientSecure client;
    client.setInsecure(); // Zertifikatsprüfung überspringen für einfachere Wartung
    HTTPClient http;

    String url = "https://api.github.com/repos/" + GITHUB_USER + "/" + GITHUB_REPO + "/releases/latest";
    
    Serial.println("[OTA] Prüfe auf Updates: " + url);
    
    http.begin(client, url);
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
                    newVersion = tag_name;
                    break;
                }
            }

            if (newVersion != "" && newVersion != CURRENT_VERSION) {
                Serial.printf("[OTA] Neue Version verfügbar: %s (Aktuell: %s)\n", newVersion.c_str(), CURRENT_VERSION.c_str());
                http.end();
                return newVersion;
            } else {
                Serial.println("[OTA] Software ist auf dem neuesten Stand.");
            }
        } else {
            Serial.println("[OTA] JSON Parsing fehlgeschlagen!");
        }
    } else {
        Serial.printf("[OTA] HTTP Fehler: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return "";
}

String updateSoftware() {
    if (latestVersionLink == "") return "Kein Download-Link gefunden.";

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    Serial.println("[OTA] Starte Download: " + latestVersionLink);
    
    // GitHub Assets leiten oft auf objects.githubusercontent.com um
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(client, latestVersionLink);
    http.addHeader("User-Agent", userAgent);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return "Download fehlgeschlagen (HTTP " + String(httpCode) + ")";
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        http.end();
        return "Ungültige Dateigröße!";
    }

    // OTA Update Prozess starten
    if (!Update.begin(contentLength)) {
        http.end();
        return "Nicht genug Platz für das Update!";
    }

    Serial.println("[OTA] Schreibe Flash...");
    WiFiClient* stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);

    if (written != contentLength) {
        http.end();
        return "Schreibfehler: " + String(written) + "/" + String(contentLength) + " Bytes geschrieben.";
    }

    if (!Update.end()) {
        http.end();
        return "Update.end() fehlgeschlagen! Fehler: " + String(Update.getError());
    }

    if (!Update.isFinished()) {
        http.end();
        return "Update nicht ordnungsgemäß beendet!";
    }

    Serial.println("[OTA] Update erfolgreich abgeschlossen. Starte neu...");
    http.end();
    
    // Wir geben die Info zurück, aber der ESP sollte idealerweise neu starten
    delay(1000);
    ESP.restart(); 
    
    return "Erfolg"; 
}