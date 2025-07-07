#ifndef WATERING_LOGIC_H
#define WATERING_LOGIC_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include "pinConfig.h"
#include "helpers.h"

#include "watering-logic/common.h"
#include "watering-logic/interval-max.h"
#include "watering-logic/moisture-cycles.h"
#include "watering-logic/interval-duration.h"
#include "weather/rain-recheck.h"
#include "weather/weather.h"

extern int cachedRainChance;
extern bool alreadyCheckedToday;
extern int lastCheckedDay;

int readSoilMoisture(int analogPin, int zoneId);
int getRelayPin(int zoneId);
int getSensorPin(int zoneId);
String getZoneFilename(int zoneId);

// Aktuális nap és óra
int currentDayOfYear() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return -1;
  return timeinfo.tm_yday + 1;
}
int currentHour() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return -1;
  return timeinfo.tm_hour;
}

void checkIntelligentDryCycleZones() {
  int today = currentDayOfYear();
  int hour = currentHour();
  if (today < 0 || hour < 0) return;

  for (int zoneId = 1; zoneId <= 6; ++zoneId) {
    String filename = getZoneFilename(zoneId);
    if (!LittleFS.exists(filename)) continue;

    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file);
    file.close();

    if (doc["mode"] != "intelligent-dry-cycle") continue;

    int dryMin = doc["dryRangeMin"] | 25;
    int dryMax = doc["dryRangeMax"] | 40;
    int requiredDryHours = doc["requiredDryHours"] | 72;
    int dryCycleDays = doc["dryCycleDays"] | 3;
    int maxMoisture = doc["maxMoisture"] | 65;
    int lastDay = doc["lastWateredDay"] | 0;

    if ((today - lastDay) < dryCycleDays) continue;

    if (!isWithinWateringWindowNoCycle(doc, hour, zoneId)) continue;

    int sensor = getSensorPin(zoneId);
    int relay = getRelayPin(zoneId);
    int moisture = readSoilMoisture(sensor, zoneId);

    bool updated = false;

    // 🌦️ Weather logic
    if (doc.containsKey("weather") &&
        handleRainForecast(doc["weather"], zoneId, moisture, maxMoisture, sensor, relay)) {
      doc["lastWateredDay"] = today;
      updated = true;
    } else {
      // 📖 Naplófájl olvasás
      String historyFile = "/drylog_" + String(zoneId) + ".json";
      if (!LittleFS.exists(historyFile)) continue;

      File hFile = LittleFS.open(historyFile, "r");
      DynamicJsonDocument hist(4096);
      deserializeJson(hist, hFile);
      hFile.close();

      int dryHours = 0;
      JsonArray hours = hist["hours"];
      for (JsonObject obj : hours) {
        int d = obj["day"];
        int h = obj["hour"];
        int m = obj["moisture"];
        if (d > lastDay && m >= dryMin && m <= dryMax) {
          dryHours++;
        }
      }

      if (dryHours >= requiredDryHours) {
        Serial.printf("[INT-DRY] Zóna %d: %d száraz óra után locsolás indul\n", zoneId, dryHours);
        digitalWrite(relay, LOW);
        digitalWrite(RELAY_PUMP, LOW);
        activeZones.push_back(WateringZone{zoneId, relay, sensor, maxMoisture});
        doc["lastWateredDay"] = today;
        updated = true;
      } else {
        Serial.printf("[INT-DRY] Zóna %d: csak %d száraz óra – nincs locsolás\n", zoneId, dryHours);
      }
    }

    // 💾 Mentés, ha frissült a lastWateredDay
    if (updated) {
      File outFile = LittleFS.open(filename, "w");
      serializeJson(doc, outFile);
      outFile.close();
    }
  }
}




void logMoistureForDryZones() {
  int today = currentDayOfYear();
  int hour = currentHour();
  if (today < 0 || hour < 0) return;

  for (int zoneId = 1; zoneId <= 6; ++zoneId) {
    String filename = getZoneFilename(zoneId);
    if (!LittleFS.exists(filename)) continue;

    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, file);
    file.close();

    if (doc["mode"] != "intelligent-dry-cycle") continue;

    int moisture = readSoilMoisture(getSensorPin(zoneId), zoneId);

    // Log hozzáadása
    String histFile = "/drylog_" + String(zoneId) + ".json";
    DynamicJsonDocument hist(4096);
    if (LittleFS.exists(histFile)) {
      File f = LittleFS.open(histFile, "r");
      deserializeJson(hist, f);
      f.close();
    }

    if (!hist.containsKey("hours")) hist["hours"] = JsonArray();

    JsonArray hours = hist["hours"];
    JsonObject entry = hours.createNestedObject();
    entry["day"] = today;
    entry["hour"] = hour;
    entry["moisture"] = moisture;

    // mentés
    File f = LittleFS.open(histFile, "w");
    serializeJson(hist, f);
    f.close();
  }
}


#endif
