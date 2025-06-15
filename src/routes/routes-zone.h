#ifndef ROUTES_ZONE_H
#define ROUTES_ZONE_H

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "helpers.h"

void registerZoneRoutes(AsyncWebServer& server) {

  // 🌱 Kalibráció beállítása (POST /api/set-calibration)
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

    DynamicJsonDocument resDoc(128);
    resDoc["status"] = "calibrated";
    resDoc["type"] = type;
    resDoc["value"] = value;

    String response;
    serializeJson(resDoc, response);
    request->send(200, "application/json", response);
  });

  // 🌿 Kalibráció lekérdezés (GET /api/get-calibration?zone=1)
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

  // 🌾 Zóna konfiguráció lekérés (GET /api/zone-config?zone=1)
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

  // 💾 Zóna konfiguráció mentése (POST /api/zone-config)
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

    serializeJsonPretty(doc, Serial);
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
}

#endif
