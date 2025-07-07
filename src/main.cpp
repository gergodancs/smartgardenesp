#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "pinConfig.h"
#include <vector>
#include "wateringLogic.h"
#include "helpers.h"
#include "wifiManager.h"
#include "routes/routes-manual-watering.h"
#include "routes/routes-wifi.h"
#include "routes/routes-zone.h"
#include "routes/routes-config.h"
#include "weather/weather.h"


const char* ssid = "SmartGarden";
const char* password = "12345678";

AsyncWebServer server(80);

// Zóna adatszerkezet – tárolja a locsolt zónát és a hozzá tartozó adatokat

std::vector<WateringZone> activeZones;

// Nedvesség olvasása analóg szenzorból (még kalibrálni kell!)
int readSoilMoisture(int analogPin, int zoneId) {
  int raw = analogRead(analogPin);
  String filename = getZoneFilename(zoneId);
  int dry = 3000, wet = 1200; // fallback default

  bool calibrated = false;

  if (LittleFS.exists(filename)) {
    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, file);
    file.close();

    if (doc.containsKey("dryValue") && doc.containsKey("wetValue")) {
      dry = doc["dryValue"];
      wet = doc["wetValue"];
      calibrated = true;
    }
  }

  if (!calibrated) {
    return -1; // jelzi, hogy nincs kalibrált szenzor
  }

  int percent = map(raw, dry, wet, 0, 100);
  return constrain(percent, 0, 100);
}

void checkWeatherLogicIfNeeded(unsigned long now);
void checkSchedulesIfNeeded(unsigned long now);
void updateActiveZones(unsigned long now);

void setup() {
  configTime(3600 * 1, 0, "pool.ntp.org"); // UTC+1 (pl. Central European Time)
  Serial.begin(115200);

  // Kimenetek beállítása
  pinMode(RELAY_PUMP, OUTPUT); digitalWrite(RELAY_PUMP, HIGH);
  pinMode(RELAY_ZONE_1, OUTPUT); digitalWrite(RELAY_ZONE_1, HIGH);
  pinMode(RELAY_ZONE_2, OUTPUT); digitalWrite(RELAY_ZONE_2, HIGH);
  pinMode(RELAY_ZONE_3, OUTPUT); digitalWrite(RELAY_ZONE_3, HIGH);
  pinMode(RELAY_ZONE_4, OUTPUT); digitalWrite(RELAY_ZONE_4, HIGH);
  pinMode(RELAY_ZONE_5, OUTPUT); digitalWrite(RELAY_ZONE_5, HIGH);
  pinMode(RELAY_ZONE_6, OUTPUT); digitalWrite(RELAY_ZONE_6, HIGH);

  // Fájlrendszer elindítása
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount sikertelen!");
    return;
  }
  Serial.println("LittleFS mount OK");

  // Statikus fájlok kiszolgálása (React UI)
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Wi-Fi indítása
  setupWiFi();

  registerZoneRoutes(server);
  registerWiFiRoutes(server);
  registerManualWateringRoutes(server);
  registerConfigRoutes(server);

  // GET /api/active-zones → pl. [2, 4]
server.on("/api/active-zones", HTTP_GET, [](AsyncWebServerRequest *request){
  DynamicJsonDocument doc(256);
  JsonArray arr = doc.to<JsonArray>();
  for (auto& z : activeZones) {
    arr.add(z.zoneId);
  }
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
});

  server.begin();
  Serial.println("Web szerver elindítva");
}

void loop() {
  unsigned long now = millis();
  checkWeatherLogicIfNeeded(now);
  checkSchedulesIfNeeded(now);
  updateActiveZones(now);
}


void checkWeatherLogicIfNeeded(unsigned long now) {
  static unsigned long lastWeatherCheck = 0;
  if (now - lastWeatherCheck < 10 * 60 * 1000) return;

  bool anyZoneUsesWeather = false;

  for (int i = 1; i <= 6; ++i) {
    String filename = "/zone" + String(i) + ".json";
    if (!LittleFS.exists(filename)) continue;

    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, file);
    file.close();

    if (doc.containsKey("weather") && doc["weather"]["enabled"] == true) {
      anyZoneUsesWeather = true;
      break;
    }
  }

  if (anyZoneUsesWeather) {
    Serial.println("🌤️ Weather logic active – fetching forecast...");
    fetchWeatherForecast();
  } else {
    Serial.println("☁️ No zones use weather logic – skipping forecast.");
  }

  lastWeatherCheck = now;
}

void checkSchedulesIfNeeded(unsigned long now) {
  static unsigned long lastScheduleCheck = 0;
  if (now - lastScheduleCheck < 30000) return;

  checkScheduledWatering();
  checkIntervalMaxZones();
  checkIntervalDurationZones();
  checkIntelligentDryCycleZones();
  logMoistureForDryZones();

  lastScheduleCheck = now;
}

void updateActiveZones(unsigned long now) {
  static unsigned long lastMoistureCheck = 0;
  if (now - lastMoistureCheck < 5000) return;

  for (auto it = activeZones.begin(); it != activeZones.end(); ) {
    bool shouldRemove = false;

    if (it->durationMillis > 0 && now - it->startTime >= it->durationMillis) {
      Serial.printf("⏹️ Zóna %d locsolás vége (idő letelt)\n", it->zoneId);
      digitalWrite(it->relayPin, HIGH);
      shouldRemove = true;
    } else if (it->durationMillis == 0) {
      int moisture = readSoilMoisture(it->sensorPin, it->zoneId);
      Serial.printf("🟢 Zóna %d aktív – Nedvesség: %d%% (Cél: %d%%)\n",
                    it->zoneId, moisture, it->maxMoisture);

      if (moisture >= it->maxMoisture) {
        Serial.printf("⏹️ Zóna %d locsolás vége – Nedvesség elérte a célt (%d%%)\n",
                      it->zoneId, moisture);
        digitalWrite(it->relayPin, HIGH);
        shouldRemove = true;
      }
    }

    if (shouldRemove) {
      it = activeZones.erase(it);
    } else {
      ++it;
    }
  }

  if (activeZones.empty()) {
    digitalWrite(RELAY_PUMP, HIGH);
  }

  lastMoistureCheck = now;
}

