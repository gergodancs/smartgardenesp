#include "moisture-cycles.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "../pinConfig.h"
#include "../../include/helpers.h"
#include "common.h"
#include "../weather/rain-recheck.h" 
#include "../weather/weather.h"

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
    bool updated = false;  // ÚJ: mentés flag

    for (JsonObject cycle : cycles) {
      int minMoisture = cycle["minMoisture"];
      int maxMoisture = cycle["maxMoisture"];
      int dryCycle = cycle["dryCycle"];
      int startMonth = cycle["startMonth"];
      int startDay = cycle["startDay"];

      // ⏱️ időablak
      if (!isWithinWateringWindow(cycle, hour, zoneId)) continue;

      // 📅 nap ellenőrzés
struct tm date = {0};
date.tm_year = 2024 - 1900;
date.tm_mon = startMonth - 1;
date.tm_mday = startDay;
mktime(&date);
int cycleStart = date.tm_yday + 1;
int daysSinceStart = today - cycleStart;
if (daysSinceStart < 0) continue;  // Csak akkor számoljuk, ha már elindult a ciklus


      // 🌱 nedvesség
int sensor = getSensorPin(zoneId);
int relay = getRelayPin(zoneId);
int moisture = readSoilMoisture(sensor, zoneId);

// ⏱️ Ellenőrizd, mikor volt utoljára locsolva
time_t now = time(nullptr);
time_t lastWatered = doc["lastWateredTime"] | 0;
int dryCycleHours = dryCycle;

bool dryCycleExpired = (dryCycleHours == 0 || now - lastWatered >= dryCycleHours * 3600);
bool tooDry = (moisture < minMoisture);

if (dryCycleExpired) {
  if (tooDry) {
    // 🌦️ Weather logic
    if (doc.containsKey("weather") &&
        handleRainForecast(doc["weather"], zoneId, moisture, maxMoisture, sensor, relay)) {
      doc["lastWateredTime"] = now;
      updated = true;
      break;
    }

    // 💧 normál locsolás
    Serial.printf("[MOISTURE] Zóna %d locsolás indul (%d%% < %d%%)\n", zoneId, moisture, maxMoisture);
    digitalWrite(relay, LOW);
    digitalWrite(RELAY_PUMP, LOW);
    activeZones.push_back(WateringZone{zoneId, relay, sensor, maxMoisture});
    doc["lastWateredTime"] = now;
    updated = true;
    break;
  } else {
    Serial.printf("[MOISTURE] Zóna %d – száraz ciklus lejárt, de a nedvesség (%d%%) még elegendő\n", zoneId, moisture);
  }
}

    }

    // 🔄 Mentés, ha történt locsolás
    if (updated) {
      File outFile = LittleFS.open(filename, "w");
      serializeJson(doc, outFile);
      outFile.close();
    }
  }
}
