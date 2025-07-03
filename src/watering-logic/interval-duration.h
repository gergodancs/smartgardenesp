#ifndef INTERVAL_DURATION_WATERING_H
#define INTERVAL_DURATION_WATERING_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../pinConfig.h"
#include "../include/helpers.h"
#include "common.h"

void checkIntervalDurationZones() {
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

      // Időablak-ellenőrzés
      if (cycle.containsKey("startHour") && cycle.containsKey("endHour")) {
        int startHour = cycle["startHour"];
        int endHour = cycle["endHour"];
        int now = currentHour();
        if (!(now >= startHour && now < endHour)) {
          Serial.printf("⏱️  Zóna %d ciklusa kihagyva: %d óra nincs az időablakban (%d–%d)\n", zoneId, now, startHour, endHour);
          continue;
        }
      }

      if (daysSinceStart < 0 || intervalDays == 0 || durationMinutes == 0 || (today - lastDay < intervalDays)) continue;

      Serial.printf("[INT-DURATION] Zóna %d locsolás indul (%d perc)\n", zoneId, durationMinutes);
      int relay = getRelayPin(zoneId);
      int sensor = getSensorPin(zoneId);

      digitalWrite(relay, LOW);
      digitalWrite(RELAY_PUMP, LOW);

      WateringZone zone = {
        zoneId,
        relay,
        sensor,
        999,
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
