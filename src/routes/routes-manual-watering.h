#ifndef ROUTES_MANUAL_H
#define ROUTES_MANUAL_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "helpers.h"
#include <LittleFS.h>
#include "pinConfig.h"
#include "wateringLogic.h"
extern std::vector<WateringZone> activeZones;

void registerManualWateringRoutes(AsyncWebServer& server) {
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
  
      int maxMoisture = doc["maxMoisture"] | 60;
      int relayPin = getRelayPin(zoneId);
      int sensorPin = getSensorPin(zoneId);
  
      int moisture = readSoilMoisture(sensorPin, zoneId);
      unsigned long duration = 0;
  
      if (moisture == -1) {
        duration = 15 * 60 * 1000UL; // 15 perc millis-ben
        Serial.printf("Zóna %d: nincs kalibrált szenzor – időzített locsolás (%lu ms)\n", zoneId, duration);
      } else {
        Serial.printf("Zóna %d: szenzoros locsolás indul (nedvesség: %d%%)\n", zoneId, moisture);
      }
  
      digitalWrite(relayPin, LOW);
      digitalWrite(RELAY_PUMP, LOW);
  
      activeZones.push_back({
        zoneId,
        relayPin,
        sensorPin,
        maxMoisture,
        millis(),
        duration
      });
  
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
            digitalWrite(z.relayPin, HIGH);
            Serial.printf("Zóna %d locsolás leállítva\n", zoneId);
            return true;
          }
          return false;
        }),
        activeZones.end()
      );
  
      if (activeZones.empty()) {
        digitalWrite(RELAY_PUMP, HIGH);
      }
  
      request->send(200, "application/json", "{\"status\":\"stopped\"}");
    });
  }
  
#endif
