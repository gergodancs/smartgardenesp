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

#endif
