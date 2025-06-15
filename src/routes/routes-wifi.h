#ifndef ROUTES_WIFI_H
#define ROUTES_WIFI_H

#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "wifiManager.h"

void registerWiFiRoutes(AsyncWebServer& server) {
  server.on("/api/wifi-scan", HTTP_GET, handleWiFiScanRequest);

  server.on("/api/wifi-connect", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
  }, NULL, handleWiFiConnectRequest);

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
}

#endif
