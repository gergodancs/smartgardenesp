#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "pinConfig.h"
#include <vector>
#include "wateringLogic.h"
#include "helpers.h"
#include "wifiManager.h"
#include "routes/routes-manual-watering.h"
#include "routes/routes-wifi.h"
#include "routes/routes-zone.h"


const char* ssid = "SmartGarden";
const char* password = "12345678";

AsyncWebServer server(80);

// Zóna adatszerkezet – tárolja a locsolt zónát és a hozzá tartozó adatokat

std::vector<WateringZone> activeZones;

// Nedvesség olvasása analóg szenzorból (még kalibrálni kell!)
int readSoilMoisture(int analogPin, int zoneId) {
  int raw = analogRead(analogPin);
  String filename = getZoneFilename(zoneId);
  int dry = 3000, wet = 1200; // fallback default

  bool calibrated = false;

  if (LittleFS.exists(filename)) {
    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, file);
    file.close();

    if (doc.containsKey("dryValue") && doc.containsKey("wetValue")) {
      dry = doc["dryValue"];
      wet = doc["wetValue"];
      calibrated = true;
    }
  }

  if (!calibrated) {
    return -1; // jelzi, hogy nincs kalibrált szenzor
  }

  int percent = map(raw, dry, wet, 0, 100);
  return constrain(percent, 0, 100);
}


void setup() {
  configTime(3600 * 1, 0, "pool.ntp.org"); // UTC+1 (pl. Central European Time)
  Serial.begin(115200);

  // Kimenetek beállítása
  pinMode(RELAY_PUMP, OUTPUT); digitalWrite(RELAY_PUMP, LOW);
  pinMode(RELAY_ZONE_1, OUTPUT); digitalWrite(RELAY_ZONE_1, LOW);
  pinMode(RELAY_ZONE_2, OUTPUT); digitalWrite(RELAY_ZONE_2, LOW);
  pinMode(RELAY_ZONE_3, OUTPUT); digitalWrite(RELAY_ZONE_3, LOW);
  pinMode(RELAY_ZONE_4, OUTPUT); digitalWrite(RELAY_ZONE_4, LOW);
  pinMode(RELAY_ZONE_5, OUTPUT); digitalWrite(RELAY_ZONE_5, LOW);
  pinMode(RELAY_ZONE_6, OUTPUT); digitalWrite(RELAY_ZONE_6, LOW);

  // Fájlrendszer elindítása
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount sikertelen!");
    return;
  }
  Serial.println("LittleFS mount OK");

  // Statikus fájlok kiszolgálása (React UI)
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Wi-Fi indítása
  setupWiFi();

  registerZoneRoutes(server);
  registerWiFiRoutes(server);
  registerManualWateringRoutes(server);
    

  server.begin();
  Serial.println("Web szerver elindítva");
}

void loop() {
  static unsigned long lastMoistureCheck = 0;
  static unsigned long lastScheduleCheck = 0;
  unsigned long now = millis();

  if (now - lastScheduleCheck > 60000) {
    checkScheduledWatering();
    checkIntervalMaxZones();
    checkIntervalDurationZones();
    checkIntelligentDryCycleZones();
    logMoistureForDryZones();
    lastScheduleCheck = now;
  }

  if (now - lastMoistureCheck > 5000) {
    for (auto it = activeZones.begin(); it != activeZones.end(); ) {
      bool shouldRemove = false;

      // ⏱️ interval-duration kezelés
      if (it->durationMillis > 0 && now - it->startTime >= it->durationMillis) {
        Serial.printf("⏹️ Zóna %d locsolás vége (idő letelt)\n", it->zoneId);
        digitalWrite(it->relayPin, LOW);
        shouldRemove = true;
      }

      // 💧 moisture-based ellenőrzés
      else if (it->durationMillis == 0) {
        int moisture = readSoilMoisture(it->sensorPin, it->zoneId);

        // 🔎 extra log minden 5 másodpercben, ha moisture-alapú locsolás aktív
        Serial.printf("🟢 Zóna %d aktív – Nedvesség: %d%% (Cél: %d%%)\n",
                      it->zoneId, moisture, it->maxMoisture);

        if (moisture >= it->maxMoisture) {
          Serial.printf("⏹️ Zóna %d locsolás vége – Nedvesség elérte a célt (%d%%)\n",
                        it->zoneId, moisture);
          digitalWrite(it->relayPin, LOW);
          shouldRemove = true;
        }
      }

      if (shouldRemove) {
        it = activeZones.erase(it);
      } else {
        ++it;
      }
    }

    if (activeZones.empty()) {
      digitalWrite(RELAY_PUMP, LOW);
    }

    lastMoistureCheck = now;
  }
}


