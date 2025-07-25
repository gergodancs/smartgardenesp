#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESPAsyncWebServer.h>

void setupWiFi(); // Dual mód WiFi setup (AP + STA)
void handleWiFiScanRequest(AsyncWebServerRequest *request);
void handleWiFiConnectRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void registerWiFiEventHandler();
#endif
