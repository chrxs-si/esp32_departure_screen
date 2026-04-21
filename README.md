# 🚍 ESP32 Live-Abfahrtsmonitor für Berlin

Du möchtest nie wieder den Bus oder die Bahn verpassen? Baue dir mit diesem Code dein eigenes Abfahrtdisplay, welche Live_Daten des VBB und der BVG direkt bei dir zu Hause anzeigt!

![GitHub release (latest by date)](https://img.shields.io/github/v/release/chrxs-si/esp32_departure_screen)
![Platform](https://img.shields.io/badge/platform-ESP32-orange)
![Framework](https://img.shields.io/badge/framework-PlatformIO-green)

---

## 💡 Die Idee
Ich habe das Glück, direkt vor meine Wohnung, eine Bus Haltestelle zu haben. Und trotz 30 Sekunden Geh-Zeit zur Haltestelle, verpassen meine Mitbewohner regelmäßig den Bus (Und ich auch). Das woltle ich ändern, indem ein Display, live anzeigt wann (und ob) der Bus kommt.
Hier stelle ich den Code zur verfügung um die Anzeige leicht nachbauen lassen zu können. Alternativ möchte ich die Displays auf Etsy verkaufen, was wahrscheinlich komfortabler ist als sich mit einem ESP32 rumzuschlagen: https://departureboard.etsy.com
Das Beste daran: Dank des integrierten Web-Setups muss keine einzige Zeile Code geändert werden, um eine Haltestelle und andere Eintellungen zu konfigurieren. Läuft alles super einfach über ein Handy oder Laptop!

## ✨ Features
* **Echtzeit-Daten:** Direkte Anbindung an die VBB/BVG-Schnittstellen.
* **Verspätungs-Check:** Zeigt nicht nur den Fahrplan, sondern berechnet die tatsächliche Abfahrtszeit inklusive Verspätung!
* **Intelligentes Filtern:** Du willst nur die U-Bahn sehen, aber nicht den Bus? Du kannst expliziet nach bestimmten Linien filtern.
* **Automatisches Web-Setup:** Beim Start spannt der ESP32 ein eigenes WLAN auf. Über eine komfortable Weboberfläche wählst du dein WLAN, deine Haltestelle und deine Wunschfarben aus. Die Konfigurationen lassen sich auch später jederzeit ändern.
* **Multitalent:** Neben Abfahrten bietet das Display (optional) Wetterinfos, Temperaturanzeige und eine Uht Anzeige.
* **Day/Night Dimming:** Die Helligkeit passt sich automatisch an (tagsüber hell, nachts dezent), damit dein Wohnzimmer nicht zur Landebahn wird.

## 🛠 Hardware
Für dieses Projekt habe ich folgende Komponenten genutzt:

* **ESP32**
* **2x HUB75 LED Panels**
* **14x Dupont-Kabel** (weiblich-weiblich)
* **Netzteil (5V):** Mind. 4A empfohlen, da die Panels bei hoher Helligkeit viel Strom benötigen.
* Weitere Komponeten zum verstecken der Kabel und des ESP32.

## 🚀 Installation & Software
Das Projekt wurde mit **PlatformIO** entwickelt.

1.  **Repository klonen:**
    ```bash
    git clone [https://github.com/dein-nutzername/esp32-transport-monitor.git](https://github.com/dein-nutzername/esp32-transport-monitor.git)
    ```
2.  **In PlatformIO öffnen:** Öffne den Ordner in VS Code mit installiertem PlatformIO Plugin.
3.  **Upload:** Verbinde deinen ESP32 und drücke auf "Upload". Die benötigten Bibliotheken (`ArduinoJson`, `ESP32-HUB75-MatrixPanel-I2S-DMA`, etc.) werden automatisch geladen.

## ⚙️ Konfiguration (Web-Interface)
Du musst im Code **keine** WLAN-Daten hinterlegen!
1.  Nach dem Flashen startet der ESP32 im **Setup-Modus**.
2.  Suche mit deinem Handy nach dem WLAN `SETUP-XXX` und verbinde dich (Passwort wird auf dem Display angezeigt).
3.  Im Browser öffnet sich automatisch das Menü (oder rufe `192.168.4.1` auf).
4.  Wähle dein WLAN, gib deine Haltestelle ein und konfiguriere die Optik.
5.  Speichern – der ESP32 startet neu und lädt die Live-Daten.
6.  Ändere die Konfiguration jederzeit indem du dich wieder mit dem WLAN des ESP32 verbindest. 

## 🎨 Personalisierung
Im Web-Interface kannst du folgende Einstellungen vornehmen:
* **Farben:** Wähle separate Farben für Abfahrten und die Uhrzeit.
* **Stations-Alias:** Ersetze lange Stationsnamen durch eigene Abkürzungen (z.B. "Warschauer Str." zu "Warschauer").
* **Offset:** Stell ein, wie viele Minuten du zur Haltestelle läufst, um zu knappe Abfahrten auszublenden.

## 🛠 Technische Details

### 🔄 Automatisierte Updates
Damit das Display immer auf dem neuesten Stand bleibt, verfügt es über eine integrierte Update-Logik:
* **Täglicher Check:** Das System prüft jede Nacht um 04:00 Uhr automatisch, ob eine neue Version vorliegt.
* **Automatischer Prozess:** Wird ein Release erkannt, stößt der ESP32 den `updateSystem`-Prozess an, um neue Features oder Bugfixes einzuspielen.

### 🌐 Genutzte API-Schnittstellen
Der Monitor ist ein echtes Kommunikationstalent und kombiniert Daten aus verschiedenen Quellen:
* **ÖPNV-Daten (VBB & BVG):** Die Live-Abfahrten werden über die REST-Schnittstellen von `v6.vbb.transport.rest` und `v6.bvg.transport.rest` abgerufen.
* **Haltestellensuche:** Für die komfortable Suche nach Stationen im Web-Interface wird die BVG-Schnittstelle genutzt.
* **Wetterdaten:** Basierend auf den GPS-Koordinaten der gewählten Haltestelle werden aktuelle Wetterinformationen bezogen.
* **Zeitsynchronisierung:** Damit die Uhr sekundengenau geht, erfolgt ein regelmäßiger Abgleich mit NTP-Servern wie `pool.ntp.org`.

## 📂 grobe Projektstruktur
* `main.cpp`: Hauptsteuerung, Wetter-Updates und Helligkeitsregelung.
* `transport_data.cpp`: API-Anbindung an VBB/BVG und Parsing der Abfahrten.
* `wifi_setup.cpp`: Webserver-Logik, Captive Portal und Einstellungs-Management (Preferences).

---
*Hinweis: Dieses Projekt ist ein steht in keiner offiziellen Verbindung zur BVG oder dem VBB.*

<p align="center">
  Made in Berlin (and for Berlin).
</p>
<p align="center">
  ❤️🧡💛💚💙💜
</p>