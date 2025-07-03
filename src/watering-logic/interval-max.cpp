#include "interval-max.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "../include/helpers.h"
#include "../pinConfig.h"
#include "common.h"

#include <vector>

int readSoilMoisture(int analogPin, int zoneId);

void checkIntervalMaxZones() {
  int today = currentDayOfYear();
  if (today < 0) return;

  for (int zoneId = 1; zoneId <= 6; ++zoneId) {
    String filename = getZoneFilename(zoneId);
    if (!LittleFS.exists(filename)) continue;

    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, file);
    file.close();

    if (doc["mode"] != "interval-max") continue;

    JsonArray cycles = doc["cycles"];
    bool updated = false;

    for (JsonObject cycle : cycles) {
      int intervalDays = cycle["intervalDays"] | 0;
      int maxMoisture = cycle["maxMoisture"] | 60;
      int lastDay = cycle["lastWateredDay"] | 0;
      int startMonth = cycle["startMonth"];
      int startDay = cycle["startDay"];

      struct tm date = {0};
      date.tm_year = 2024 - 1900;
      date.tm_mon = startMonth - 1;
      date.tm_mday = startDay;
      mktime(&date);
      int cycleStart = date.tm_yday + 1;
      int daysSinceStart = today - cycleStart;

      if (daysSinceStart < 0 || intervalDays == 0 || (today - lastDay < intervalDays)) continue;

      int sensor = getSensorPin(zoneId);
      int relay = getRelayPin(zoneId);
      int moisture = readSoilMoisture(sensor, zoneId);

      if (moisture < maxMoisture) {
        Serial.printf("[INT-MAX] Zóna %d locsolás indul (%d%% < %d%%)\n", zoneId, moisture, maxMoisture);
        digitalWrite(relay, LOW);
        digitalWrite(RELAY_PUMP, LOW);
        activeZones.push_back({zoneId, relay, sensor, maxMoisture});
        cycle["lastWateredDay"] = today;
        updated = true;
        break;
      }
    }

    if (updated) {
      // 🧹 Csak az aktuálisan locsolt ciklus maradjon
      JsonArray newCycles = doc.createNestedArray("cycles");
    
      for (JsonObject c : cycles) {
        if (c.containsKey("lastWateredDay") && c["lastWateredDay"] == today) {
          newCycles.add(c);
          break;
        }
      }
    
      File outFile = LittleFS.open(filename, "w");
      serializeJson(doc, outFile);
      outFile.close();
    }
    

  }
}
