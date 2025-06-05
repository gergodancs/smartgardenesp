#ifndef WATERING_LOGIC_H
#define WATERING_LOGIC_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>
#include "pinConfig.h"
#include "helpers.h"

// Külső változók, típusok
struct WateringZone {
  int zoneId;
  int relayPin;
  int sensorPin;
  int maxMoisture; // csak azonnali és interval-max módhoz
  unsigned long startTime;     // interval-duration időzítéshez
  unsigned long durationMillis; // interval-duration időzítéshez
};
extern std::vector<WateringZone> activeZones;

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

// Állapot az egyszeri napi automatikus ellenőrzéshez
bool alreadyCheckedToday = false;
int lastCheckedDay = -1;

// Automatikus locsolás moisture-cycles móddal, este 21:00-kor
void checkScheduledWatering() {
  int today = currentDayOfYear();
  int hour = currentHour();
  if (today < 0 || hour < 0) return;

  if (hour == 21 && (!alreadyCheckedToday || lastCheckedDay != today)) {
    alreadyCheckedToday = true;
    lastCheckedDay = today;

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

        struct tm date = {0};
        date.tm_year = 2024 - 1900;
        date.tm_mon = startMonth - 1;
        date.tm_mday = startDay;
        mktime(&date);
        int cycleStart = date.tm_yday + 1;

        int daysSinceStart = today - cycleStart;
        if (daysSinceStart < 0 || (dryCycle > 0 && daysSinceStart % dryCycle != 0)) continue;

        int moisture = readSoilMoisture(getSensorPin(zoneId), zoneId);
        if (moisture < maxMoisture) {
          Serial.printf("[AUTO] Zóna %d locsolás indul (%d%% < %d%%)\n", zoneId, moisture, maxMoisture);
          digitalWrite(getRelayPin(zoneId), HIGH);
          digitalWrite(RELAY_PUMP, HIGH);
          activeZones.push_back({zoneId, getRelayPin(zoneId), getSensorPin(zoneId), maxMoisture});
          break;
        }
      }
    }
  }

  if (hour == 0 && today != lastCheckedDay) {
    alreadyCheckedToday = false;
  }
}

// Automatikus locsolás interval-max alapján
void checkIntervalMaxZones() {
  int today = currentDayOfYear();
  if (today < 0) return;

  for (int zoneId = 1; zoneId <= 6; ++zoneId) {
    String filename = getZoneFilename(zoneId);
    if (!LittleFS.exists(filename)) continue;

    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file);
    file.close();

    if (doc["mode"] != "interval-max") continue;

    int intervalDays = doc["intervalDays"] | 0;
    int maxMoisture = doc["maxMoisture"] | 60;
    int lastDay = doc["lastWateredDay"] | 0;

    if (intervalDays == 0 || (today - lastDay) < intervalDays) continue;

    int sensor = getSensorPin(zoneId);
    int relay = getRelayPin(zoneId);
    int moisture = readSoilMoisture(sensor, zoneId);
    if (moisture < maxMoisture) {
      Serial.printf("[INT-MAX] Zóna %d locsolás indul (%d%% < %d%%)\n", zoneId, moisture, maxMoisture);
      digitalWrite(relay, HIGH);
      digitalWrite(RELAY_PUMP, HIGH);
      activeZones.push_back({zoneId, relay, sensor, maxMoisture});

      // újra mentjük az utolsó locsolás napját
      doc["lastWateredDay"] = today;
      File outFile = LittleFS.open(filename, "w");
      serializeJson(doc, outFile);
      outFile.close();
    }
  }
}

// Automatikus locsolás interval-duration alapján
void checkIntervalDurationZones() {
  static unsigned long lastRun = 0;
  unsigned long now = millis();
  if (now - lastRun < 60000) return; // percenként ellenőrzünk
  lastRun = now;

  int today = currentDayOfYear();
  if (today < 0) return;

  for (int zoneId = 1; zoneId <= 6; ++zoneId) {
    String filename = getZoneFilename(zoneId);
    if (!LittleFS.exists(filename)) continue;

    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file);
    file.close();

    if (doc["mode"] != "interval-duration") continue;

    int intervalDays = doc["intervalDays"] | 0;
    int durationMinutes = doc["durationMinutes"] | 0;
    int lastDay = doc["lastWateredDay"] | 0;

    if (intervalDays == 0 || durationMinutes == 0 || (today - lastDay) < intervalDays) continue;

    Serial.printf("[INT-DURATION] Zóna %d locsolás indul (%d perc)\n", zoneId, durationMinutes);
    int relay = getRelayPin(zoneId);
    int sensor = getSensorPin(zoneId);

    digitalWrite(relay, HIGH);
    digitalWrite(RELAY_PUMP, HIGH);

    WateringZone zone = {
      zoneId, relay, sensor,
      999, // maxMoisture (nem számít itt)
      millis(),
      (unsigned long)durationMinutes * 60000
    };
    activeZones.push_back(zone);

    // mentés
    doc["lastWateredDay"] = today;
    File outFile = LittleFS.open(filename, "w");
    serializeJson(doc, outFile);
    outFile.close();
  }
}


#endif
