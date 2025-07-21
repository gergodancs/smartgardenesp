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
  // Azonnali locsolás indítása (React fetch: /api/water-now?zone=3)
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
    String mode;

    if (moisture == -1) {
      duration = 15 * 60 * 1000UL; // 15 perc
      mode = "timed";
      Serial.printf("Zóna %d: nincs kalibrált szenzor – időzített locsolás (%lu ms)\n", zoneId, duration);
    } else {
      mode = "moisture";
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

    DynamicJsonDocument res(128);
    res["status"] = "watering";
    res["zoneId"] = zoneId;
    res["mode"] = mode;
    if (moisture != -1) res["moisture"] = moisture;

    String response;
    serializeJson(res, response);
    request->send(200, "application/json", response);
  });

  // Azonnali locsolás leállítása (React fetch: /api/water-stop?zone=3)
  server.on("/api/water-stop", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("zone")) {
      request->send(400, "application/json", "{\"error\":\"Missing zone parameter\"}");
      return;
    }

    int zoneId = request->getParam("zone")->value().toInt();

    bool stoppedAny = false;
    activeZones.erase(
      std::remove_if(activeZones.begin(), activeZones.end(), [zoneId, &stoppedAny](WateringZone z){
        if (z.zoneId == zoneId) {
          digitalWrite(z.relayPin, HIGH);
          Serial.printf("Zóna %d locsolás leállítva\n", zoneId);
          stoppedAny = true;
          return true;
        }
        return false;
      }),
      activeZones.end()
    );

    if (stoppedAny && activeZones.empty()) {
      digitalWrite(RELAY_PUMP, HIGH);
    }

    request->send(200, "application/json", "{\"status\":\"stopped\"}");
  });
}

#endif
