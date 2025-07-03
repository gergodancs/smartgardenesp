#include "moisture-cycles.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "../pinConfig.h"
#include "../../include/helpers.h"
#include "common.h"

#include <vector>
extern std::vector<WateringZone> activeZones;
int readSoilMoisture(int analogPin, int zoneId);
int getRelayPin(int zoneId);
int getSensorPin(int zoneId);
String getZoneFilename(int zoneId);

bool alreadyCheckedToday = false;
int lastCheckedDay = -1;

void checkScheduledWatering() {
  int today = currentDayOfYear();
  int hour = currentHour();
  if (today < 0 || hour < 0) return;


    for (int zoneId = 1; zoneId <= 6; ++zoneId) {
      String filename = getZoneFilename(zoneId);
      if (!LittleFS.exists(filename)) continue;

      File file = LittleFS.open(filename, "r");
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, file);
      file.close();

      String mode = doc["mode"] | "";
      if (mode != "moisture-cycles") continue;

      JsonArray cycles = doc["cycles"];
      for (JsonObject cycle : cycles) {
        int minMoisture = cycle["minMoisture"];
        int maxMoisture = cycle["maxMoisture"];
        int dryCycle = cycle["dryCycle"];
        int startMonth = cycle["startMonth"];
        int startDay = cycle["startDay"];
      
        // Ellenőrizzük az időablakot
        if (cycle.containsKey("startHour") && cycle.containsKey("endHour")) {
          int startHour = cycle["startHour"];
          int endHour = cycle["endHour"];
          if (!(hour >= startHour && hour < endHour)) {
            Serial.printf("⏱️  Zóna %d ciklusa kihagyva: %d óra nincs az időablakban (%d–%d)\n", zoneId, hour, startHour, endHour);
            continue;
          }
        }
      
        // Ellenőrizzük a ciklus napját
        struct tm date = {0};
        date.tm_year = 2024 - 1900;
        date.tm_mon = startMonth - 1;
        date.tm_mday = startDay;
        mktime(&date);
        int cycleStart = date.tm_yday + 1;
      
        int daysSinceStart = today - cycleStart;
        if (daysSinceStart < 0 || (dryCycle > 0 && daysSinceStart % dryCycle != 0)) continue;
      
        // Nedvesség alapján döntés
        int moisture = readSoilMoisture(getSensorPin(zoneId), zoneId);
        if (moisture < maxMoisture) {
          Serial.printf("[AUTO] Zóna %d locsolás indul (%d%% < %d%%)\n", zoneId, moisture, maxMoisture);
          digitalWrite(getRelayPin(zoneId), LOW);
          digitalWrite(RELAY_PUMP, LOW);
          activeZones.push_back({zoneId, getRelayPin(zoneId), getSensorPin(zoneId), maxMoisture});
          break;
        }
      }
  }
}
