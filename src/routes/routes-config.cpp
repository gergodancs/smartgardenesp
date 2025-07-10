#include "routes-config.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <sys/time.h>
#include <time.h>
#include <Arduino.h>

void handleRestart(AsyncWebServerRequest *request) {
  request->send(200, "application/json", "{\"status\":\"restarting\"}");
  delay(1000);
  ESP.restart();
}

void handleFactoryReset(AsyncWebServerRequest *request) {
  for (int i = 1; i <= 6; ++i) {
    String filename = "/zone" + String(i) + ".json";
    if (LittleFS.exists(filename)) LittleFS.remove(filename);
  }
  if (LittleFS.exists("/wifi.json")) LittleFS.remove("/wifi.json");
  if (LittleFS.exists("/location.json")) LittleFS.remove("/location.json");

  request->send(200, "application/json", "{\"status\":\"resetting\"}");
  delay(1000);
  ESP.restart();
}

void handleSystemConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  if (doc.containsKey("country") || doc.containsKey("city")) {
    String country = doc["country"] | "";
    String city = doc["city"] | "";

    DynamicJsonDocument locDoc(256);
    locDoc["country"] = country;
    locDoc["city"] = city;

    File locFile = LittleFS.open("/location.json", "w");
    if (locFile) {
      serializeJson(locDoc, locFile);
      locFile.close();
      Serial.println("📁 Lokáció mentve.");
    } else {
      Serial.println("❌ Nem sikerült menteni a lokációt.");
    }
  }

  request->send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleGetCurrentTime(AsyncWebServerRequest *request) {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  DynamicJsonDocument doc(128);
  char dateBuf[11];
  char timeBuf[6];

  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d",
           timeinfo->tm_year + 1900,
           timeinfo->tm_mon + 1,
           timeinfo->tm_mday);

  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d",
           timeinfo->tm_hour,
           timeinfo->tm_min);

  doc["date"] = dateBuf;
  doc["time"] = timeBuf;

  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void registerConfigRoutes(AsyncWebServer& server) {
  server.on("/api/restart", HTTP_POST, handleRestart);
  server.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
  server.on("/api/system-config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handleSystemConfig);
  server.on("/api/current-time", HTTP_GET, handleGetCurrentTime);
}
