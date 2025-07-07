#ifndef WATERING_COMMON_H
#define WATERING_COMMON_H

#include <vector>
#include <Arduino.h>
#include "pinConfig.h"

struct WateringZone {
  int zoneId;
  int relayPin;
  int sensorPin;
  int maxMoisture;
  unsigned long startTime;
  unsigned long durationMillis;
};

extern std::vector<WateringZone> activeZones;

int currentDayOfYear();
int currentHour();

inline bool isWithinWateringWindow(JsonObject obj, int currentHour, int zoneId = -1) {
  if (obj.containsKey("startHour") && obj.containsKey("endHour")) {
    int startHour = obj["startHour"];
    int endHour = obj["endHour"];
    if (!(currentHour >= startHour && currentHour < endHour)) {
      if (zoneId != -1) {
        Serial.printf("⏱️  Zóna %d ciklusa kihagyva: %d óra nincs az időablakban (%d–%d)\n", zoneId, currentHour, startHour, endHour);
      } else {
        Serial.printf("⏱️  Ciklus kihagyva: %d óra nincs az időablakban (%d–%d)\n", currentHour, startHour, endHour);
      }
      return false;
    }
  }
  return true;
}

inline bool isWithinWateringWindowNoCycle(JsonVariant config, int hour, int zoneId) {
  if (config.containsKey("startHour") && config.containsKey("endHour")) {
    int start = config["startHour"];
    int end = config["endHour"];
    if (!(hour >= start && hour < end)) {
      Serial.printf("⏱️  Zóna %d: %d óra nincs az időablakban (%d–%d) – kihagyva\n", zoneId, hour, start, end);
      return false;
    }
  }
  return true;
}

#endif
