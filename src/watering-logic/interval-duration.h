#ifndef INTERVAL_DURATION_WATERING_H
#define INTERVAL_DURATION_WATERING_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../pinConfig.h"
#include "../include/helpers.h"
#include "common.h"

extern int cachedRainChance;

void checkIntervalDurationZones() {
  int hour = currentHour();
  int today = currentDayOfYear();
  if (today < 0) return;

  for (int zoneId = 1; zoneId <= 6; ++zoneId) {
    String filename = getZoneFilename(zoneId);
    if (!LittleFS.exists(filename)) continue;

    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, file);
    file.close();

    if (doc["mode"] != "interval-duration") continue;

    JsonArray cycles = doc["cycles"];
    bool updated = false;

    for (JsonObject cycle : cycles) {
      int intervalDays = cycle["intervalDays"] | 0;
      int durationMinutes = cycle["durationMinutes"] | 0;
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

      // ⏱️ Időablak ellenőrzés
      if (!isWithinWateringWindow(cycle, hour, zoneId)) continue;

      // ⏳ Általános kihagyási feltételek
      if (daysSinceStart < 0 || intervalDays == 0 || durationMinutes == 0 || (today - lastDay < intervalDays)) continue;

      // 🌦️ Weather logika
      if (doc.containsKey("weather")) {
        JsonObject weather = doc["weather"];
        bool enabled = weather["enabled"] | false;
        int threshold = weather["rainChanceThreshold"] | 0;
        int forecastDays = weather["forecastDays"] | 1;

        if (enabled && cachedRainChance >= threshold) {
          Serial.printf("🌧️ Zóna %d kihagyva – %d%% esély esőre a következő %d napon\n", zoneId, cachedRainChance, forecastDays);
          break; // nem locsolunk, kihagyjuk
        }
      }

      // 💧 Normál időalapú locsolás
      Serial.printf("[INT-DURATION] Zóna %d locsolás indul (%d perc)\n", zoneId, durationMinutes);
      int relay = getRelayPin(zoneId);
      int sensor = getSensorPin(zoneId);

      digitalWrite(relay, LOW);
      digitalWrite(RELAY_PUMP, LOW);

      WateringZone zone = {
        zoneId,
        relay,
        sensor,
        999, // nem használunk moisture-t
        millis(),
        (unsigned long)durationMinutes * 60000
      };
      activeZones.push_back(zone);

      cycle["lastWateredDay"] = today;
      updated = true;
      break;
    }

    if (updated) {
      File outFile = LittleFS.open(filename, "w");
      serializeJson(doc, outFile);
      outFile.close();
    }
  }
}

#endif
