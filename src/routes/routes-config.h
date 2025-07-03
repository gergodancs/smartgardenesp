#ifndef ROUTES_CONFIG_H
#define ROUTES_CONFIG_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <sys/time.h>

// 🔁 ESP újraindítás
void handleRestart(AsyncWebServerRequest *request) {
  request->send(200, "application/json", "{\"status\":\"restarting\"}");
  delay(1000);
  ESP.restart();
}

// 🧨 Gyári visszaállítás
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

// 🧠 Egységes konfiguráció mentés (helyszín + dátum/idő)
void handleSystemConfig(AsyncWebServerRequest *request) {
  if (!request->hasArg("plain")) {
    request->send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }

  DynamicJsonDocument doc(512);
  deserializeJson(doc, request->arg("plain"));

  // 🌍 Lokáció mentése, ha meg van adva
  if (doc.containsKey("country") || doc.containsKey("city")) {
    String country = doc["country"] | "";
    String city = doc["city"] | "";

    DynamicJsonDocument locDoc(256);
    locDoc["country"] = country;
    locDoc["city"] = city;

    File locFile = LittleFS.open("/location.json", "w");
    serializeJson(locDoc, locFile);
    locFile.close();
  }

  request->send(200, "application/json", "{\"status\":\"ok\"}");
}

// 🕒 Aktuális idő lekérdezése
void handleGetCurrentTime(AsyncWebServerRequest *request) {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  DynamicJsonDocument doc(128);
  char dateBuf[11];  // "2025-07-02"
  char timeBuf[6];   // "14:30"

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


// 🔗 Route-ok regisztrálása
void registerConfigRoutes(AsyncWebServer& server) {
  server.on("/api/restart", HTTP_POST, handleRestart);
  server.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
  server.on("/api/system-config", HTTP_POST, handleSystemConfig);
  server.on("/api/current-time", HTTP_GET, handleGetCurrentTime);
}

#endif
