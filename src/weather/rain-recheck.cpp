#include "rain-recheck.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../include/helpers.h"
#include "../pinConfig.h"
#include "../watering-logic/common.h"
#include "log-helper.h"

std::vector<SkippedZone> rainSkippedZones;
const char* RAIN_SKIPPED_FILE = "/rain_skipped.json";

void loadRainSkippedZones() {
  rainSkippedZones.clear();

  if (!LittleFS.exists(RAIN_SKIPPED_FILE)) return;

  File file = LittleFS.open(RAIN_SKIPPED_FILE, "r");
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, file);
  file.close();

  JsonArray arr = doc["zones"];
  for (JsonObject obj : arr) {
    SkippedZone z;
    z.zoneId = obj["zoneId"];
    z.maxMoisture = obj["maxMoisture"];
    rainSkippedZones.push_back(z);
  }
}

void saveRainSkippedZones() {
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.createNestedArray("zones");

  for (const auto& z : rainSkippedZones) {
    JsonObject obj = arr.createNestedObject();
    obj["zoneId"] = z.zoneId;
    obj["maxMoisture"] = z.maxMoisture;
  }

  File file = LittleFS.open(RAIN_SKIPPED_FILE, "w");
  serializeJson(doc, file);
  file.close();
}

void addRainSkippedZone(int zoneId, int maxMoisture) {
  for (auto& z : rainSkippedZones) {
    if (z.zoneId == zoneId) return; // már benne van
  }

  SkippedZone z{zoneId, maxMoisture};
  rainSkippedZones.push_back(z);
  saveRainSkippedZones();
}

void checkRainRecheckZones() {
  if (rainSkippedZones.empty()) return;

  for (auto it = rainSkippedZones.begin(); it != rainSkippedZones.end(); ) {
    int zoneId = it->zoneId;
    int maxMoisture = it->maxMoisture;

    int sensor = getSensorPin(zoneId);
    int relay = getRelayPin(zoneId);
    int moisture = analogRead(sensor);

    Serial.printf("🌦️ Eső után újraellenőrzés – Zóna %d: %d%% nedvesség (Cél: %d%%)\n", zoneId, moisture, maxMoisture);

    if (moisture < maxMoisture) {
      digitalWrite(relay, LOW);
      digitalWrite(RELAY_PUMP, LOW);
      activeZones.push_back(WateringZone{zoneId, relay, sensor, maxMoisture});
      it = rainSkippedZones.erase(it);
      appendToLog("Zóna " + String(zoneId) + ": eső utáni újraellenőrzés – nem lett elég nedves (" + String(moisture) + "%) → locsolás indítva");
    } else {
      ++it;
    }
  }

  saveRainSkippedZones();
}
