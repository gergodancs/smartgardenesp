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

const char* ssid = "SmartGarden";
const char* password = "12345678";

AsyncWebServer server(80);

// Zóna adatszerkezet – tárolja a locsolt zónát és a hozzá tartozó adatokat

std::vector<WateringZone> activeZones;

// Nedvesség olvasása analóg szenzorból (még kalibrálni kell!)
int readSoilMoisture(int analogPin, int zoneId) {
  int raw = analogRead(analogPin);
  String filename = getZoneFilename(zoneId);
  int dry = 3000, wet = 1200; // default fallback értékek

  if (LittleFS.exists(filename)) {
    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(512);
    deserializeJson(doc, file);
    file.close();

    dry = doc["dryValue"] | dry;
    wet = doc["wetValue"] | wet;
  }

  int percent = map(raw, dry, wet, 0, 100);
  return constrain(percent, 0, 100);
}

void setup() {
  configTime(3600 * 1, 0, "pool.ntp.org"); // UTC+1 (pl. Central European Time)
  Serial.begin(115200);

  // Kimenetek beállítása
  pinMode(RELAY_PUMP, OUTPUT); digitalWrite(RELAY_PUMP, LOW);
  pinMode(RELAY_ZONE_1, OUTPUT); digitalWrite(RELAY_ZONE_1, LOW);
  pinMode(RELAY_ZONE_2, OUTPUT); digitalWrite(RELAY_ZONE_2, LOW);
  pinMode(RELAY_ZONE_3, OUTPUT); digitalWrite(RELAY_ZONE_3, LOW);
  pinMode(RELAY_ZONE_4, OUTPUT); digitalWrite(RELAY_ZONE_4, LOW);
  pinMode(RELAY_ZONE_5, OUTPUT); digitalWrite(RELAY_ZONE_5, LOW);
  pinMode(RELAY_ZONE_6, OUTPUT); digitalWrite(RELAY_ZONE_6, LOW);

  // Wi-Fi indítása


  // Fájlrendszer elindítása
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount sikertelen!");
    return;
  }
  Serial.println("LittleFS mount OK");

  // Statikus fájlok kiszolgálása (React UI)
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  setupWiFi();

  // elérhetö a hálózatok
  server.on("/api/wifi-scan", HTTP_GET, handleWiFiScanRequest);

  //csatlakozás hálózathoz
  server.on("/api/wifi-connect", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200); // dummy
  }, NULL, handleWiFiConnectRequest);
  

  //moisture calibration
  server.on("/api/set-calibration", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("zone") || !request->hasParam("type")) {
      request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
      return;
    }
  
    int zoneId = request->getParam("zone")->value().toInt();
    String type = request->getParam("type")->value(); // "dry" vagy "wet"
    int sensorPin = getSensorPin(zoneId);
    int value = analogRead(sensorPin);
  
    String filename = getZoneFilename(zoneId);
    DynamicJsonDocument doc(1024);
  
    if (LittleFS.exists(filename)) {
      File file = LittleFS.open(filename, "r");
      deserializeJson(doc, file);
      file.close();
    } else {
      doc["zoneId"] = zoneId;
    }
  
    if (type == "dry") {
      doc["dryValue"] = value;
    } else if (type == "wet") {
      doc["wetValue"] = value;
    } else {
      request->send(400, "application/json", "{\"error\":\"Invalid type\"}");
      return;
    }
  
    File file = LittleFS.open(filename, "w");
    serializeJson(doc, file);
    file.close();
  
    // ✅ Teljes válasz: típus + érték
    String response;
    DynamicJsonDocument resDoc(128);
    resDoc["status"] = "calibrated";
    resDoc["type"] = type;
    resDoc["value"] = value;
    serializeJson(resDoc, response);
  
    request->send(200, "application/json", response);
  });

  server.on("/api/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(256);
  
    if (WiFi.status() == WL_CONNECTED) {
      doc["connected"] = true;
      doc["ip"] = WiFi.localIP().toString();
      doc["ssid"] = WiFi.SSID();
      doc["rssi"] = WiFi.RSSI();
    } else {
      doc["connected"] = false;
    }
  
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  

  // mentett kalibracios ertekek olvasasa

  server.on("/api/get-calibration", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("zone")) {
      request->send(400, "application/json", "{\"error\":\"Missing zone\"}");
      return;
    }
  
    int zoneId = request->getParam("zone")->value().toInt();
    String filename = getZoneFilename(zoneId);
  
    if (!LittleFS.exists(filename)) {
      request->send(404, "application/json", "{\"error\":\"Calibration data not found\"}");
      return;
    }
  
    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(256);
    deserializeJson(doc, file);
    file.close();
  
    DynamicJsonDocument response(256);
    response["zoneId"] = zoneId;
    response["dryValue"] = doc["dryValue"] | -1;
    response["wetValue"] = doc["wetValue"] | -1;
  
    String json;
    serializeJson(response, json);
    request->send(200, "application/json", json);
  });
  

  // Zóna beállítás lekérdezés
  server.on("/api/zone-config", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("zone")) {
      request->send(400, "application/json", "{\"error\":\"Missing zone parameter\"}");
      return;
    }
    int zoneId = request->getParam("zone")->value().toInt();
    String filename = getZoneFilename(zoneId);

    if (!LittleFS.exists(filename)) {
      request->send(200, "application/json", "{}");
      return;
    }

    File file = LittleFS.open(filename, "r");
    String content = file.readString();
    file.close();
    request->send(200, "application/json", content);
  });

 // Zóna beállítás mentés
server.on("/api/zone-config", HTTP_POST, [](AsyncWebServerRequest *request){
  request->send(200, "application/json", "{\"status\":\"ok\"}");
}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  Serial.println("➡️ POST /api/zone-config hívás érkezett");

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    Serial.print("[HIBA] JSON feldolgozási hiba: ");
    Serial.println(error.c_str());
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  serializeJsonPretty(doc, Serial); // Nyomtatás, hogy mit kaptunk
  Serial.println();

  if (!doc.containsKey("zoneId")) {
    Serial.println("[HIBA] Hiányzik a zoneId kulcs!");
    request->send(400, "application/json", "{\"error\":\"Missing zoneId\"}");
    return;
  }

  int zoneId = doc["zoneId"];
  String filename = getZoneFilename(zoneId);
  Serial.printf("📝 Mentés fájlba: %s\n", filename.c_str());

  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("[HIBA] Nem sikerült megnyitni a fájlt írásra.");
    request->send(500, "application/json", "{\"error\":\"Cannot save config\"}");
    return;
  }

  serializeJson(doc, file);
  file.close();
  Serial.println("✅ Fájl mentése sikeres.");
  request->send(200, "application/json", "{\"status\":\"saved\"}");
});


  // Azonnali locsolás indítása
  server.on("/api/water-now", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("zone")) {
      request->send(400, "application/json", "{\"error\":\"Missing zone parameter\"}");
      return;
    }
    int zoneId = request->getParam("zone")->value().toInt();
    String filename = getZoneFilename(zoneId);

    if (!LittleFS.exists(filename)) {
      request->send(404, "application/json", "{\"error\":\"No config found for zone\"}");
      return;
    }

    File file = LittleFS.open(filename, "r");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file);
    file.close();

    int maxMoisture = doc["maxMoisture"] | 60; // alapérték, ha hiányzik
    int relayPin = getRelayPin(zoneId);
    int sensorPin = getSensorPin(zoneId);

    digitalWrite(relayPin, HIGH);
    digitalWrite(RELAY_PUMP, HIGH);

    activeZones.push_back({zoneId, relayPin, sensorPin, maxMoisture});
    Serial.printf("Zóna %d locsolás elindítva\n", zoneId);
    request->send(200, "application/json", "{\"status\":\"watering\"}");
  });

  // Azonnali locsolás leállítása
  server.on("/api/water-stop", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("zone")) {
      request->send(400, "application/json", "{\"error\":\"Missing zone parameter\"}");
      return;
    }
    int zoneId = request->getParam("zone")->value().toInt();

    activeZones.erase(
      std::remove_if(activeZones.begin(), activeZones.end(), [zoneId](WateringZone z){
        if (z.zoneId == zoneId) {
          digitalWrite(z.relayPin, LOW);
          Serial.printf("Zóna %d locsolás leállítva\n", zoneId);
          return true;
        }
        return false;
      }),
      activeZones.end()
    );

    if (activeZones.empty()) {
      digitalWrite(RELAY_PUMP, LOW);
    }

    request->send(200, "application/json", "{\"status\":\"stopped\"}");
  });

  server.begin();
  Serial.println("Web szerver elindítva");
}

void loop() {
  static unsigned long lastMoistureCheck = 0;
  static unsigned long lastScheduleCheck = 0;
  unsigned long now = millis();

  if (now - lastScheduleCheck > 60000) {
    checkScheduledWatering();
    checkIntervalMaxZones();
    checkIntervalDurationZones();
    checkIntelligentDryCycleZones();
    logMoistureForDryZones();
    lastScheduleCheck = now;
  }

  if (now - lastMoistureCheck > 5000) {
    for (auto it = activeZones.begin(); it != activeZones.end(); ) {
      bool shouldRemove = false;

      // interval-duration kezelés
      if (it->durationMillis > 0 && now - it->startTime >= it->durationMillis) {
        Serial.printf("Zóna %d locsolás vége (idő letelt)\n", it->zoneId);
        digitalWrite(it->relayPin, LOW);
        shouldRemove = true;
      }

      // moisture-based ellenőrzés
      else if (it->durationMillis == 0) {
        int moisture = readSoilMoisture(it->sensorPin, it->zoneId);
        if (moisture >= it->maxMoisture) {
          Serial.printf("Zóna %d nedvesség elérte a célt (%d%%)\n", it->zoneId, moisture);
          digitalWrite(it->relayPin, LOW);
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
      digitalWrite(RELAY_PUMP, LOW);
    }

    lastMoistureCheck = now;
  }
}


