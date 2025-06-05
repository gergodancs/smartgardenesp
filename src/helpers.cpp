#include "helpers.h"
#include "pinConfig.h"

String getZoneFilename(int zoneId) {
  return "/zone_" + String(zoneId) + ".json";
}

int getRelayPin(int zoneId) {
  switch (zoneId) {
    case 1: return RELAY_ZONE_1;
    case 2: return RELAY_ZONE_2;
    case 3: return RELAY_ZONE_3;
    case 4: return RELAY_ZONE_4;
    case 5: return RELAY_ZONE_5;
    case 6: return RELAY_ZONE_6;
    default: return -1;
  }
}

int getSensorPin(int zoneId) {
  switch (zoneId) {
    case 1: return SENSOR_ZONE_1;
    case 2: return SENSOR_ZONE_2;
    case 3: return SENSOR_ZONE_3;
    case 4: return SENSOR_ZONE_4;
    case 5: return SENSOR_ZONE_5;
    case 6: return SENSOR_ZONE_6;
    default: return -1;
  }
}
