#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "wifiManager.h"
#include <vector>
#include <Arduino.h>


std::vector<String> scannedSSIDs; // Globálisan elérhető lista

void preScanNetworks() {
  Serial.println("📡 Előzetes WiFi scan indul...");

  WiFi.mode(WIFI_STA); // ideiglenesen STA only
  delay(100);          // adjunk kis időt
  int n = WiFi.scanNetworks();
  Serial.printf("🔍 %d hálózat előre feltérképezve\n", n);
  for (int i = 0; i < n; ++i) {
    scannedSSIDs.push_back(WiFi.SSID(i));
  }

  WiFi.mode(WIFI_AP_STA); // vissza AP + STA módra
}

void setupWiFi() {
  WiFi.mode(WIFI_AP_STA);

  const char* apSsid = "SmartGarden";
  const char* apPassword = "12345678";
  WiFi.softAP(apSsid, apPassword);
  Serial.print("🌱 AP elérhető: ");
  Serial.println(WiFi.softAPIP());

  bool staConnected = false;

  if (LittleFS.exists("/wifi.json")) {
    File file = LittleFS.open("/wifi.json", "r");
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (!err && doc.containsKey("ssid") && doc.containsKey("password")) {
      String ssid = doc["ssid"];
      String password = doc["password"];

      WiFi.begin(ssid.c_str(), password.c_str());
      Serial.printf("📡 Csatlakozás a hálózathoz: %s\n", ssid.c_str());

      unsigned long startAttemptTime = millis();
      const unsigned long timeout = 10000;

      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
        delay(500);
        Serial.print(".");
      }

      staConnected = WiFi.status() == WL_CONNECTED;
    } else {
      Serial.println("⚠️ Hiba a wifi.json fájl beolvasásakor.");
    }
  } else {
    Serial.println("ℹ️ Nincs elmentett WiFi beállítás (wifi.json).");
    
  }

  if (staConnected) {
    Serial.println();
    Serial.print("✅ STA IP-cím: ");
    Serial.println(WiFi.localIP());

    Serial.print("📶 Csatlakozott hálózat: ");
    Serial.println(WiFi.SSID());

    Serial.print("📡 Jelerősség (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("⚠️ Nem sikerült csatlakozni STA módban.");
  }
}


void handleWiFiScanRequest(AsyncWebServerRequest *request) {
  Serial.println("📡 WiFi scan adatok küldése...");

  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.to<JsonArray>();

  for (auto& ssid : scannedSSIDs) {
    arr.add(ssid);
  }

  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}




void handleWiFiConnectRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, data);

  if (error || !doc.containsKey("ssid") || !doc.containsKey("password")) {
    request->send(400, "application/json", "{\"error\":\"Missing or invalid JSON\"}");
    return;
  }

  String ssid = doc["ssid"].as<String>();
  String password = doc["password"].as<String>();

  DynamicJsonDocument wifiConfig(256);
  wifiConfig["ssid"] = ssid;
  wifiConfig["password"] = password;

  File file = LittleFS.open("/wifi.json", "w");
  serializeJson(wifiConfig, file);
  file.close();

  request->send(200, "application/json", "{\"status\":\"saved\", \"restarting\":true}");
delay(1000);
ESP.restart();
}
