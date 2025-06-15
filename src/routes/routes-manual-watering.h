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


}

#endif