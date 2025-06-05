#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "wifiManager.h"

void setupWiFi() {
  WiFi.mode(WIFI_AP_STA);

  const char* apSsid = "SmartGarden";
  const char* apPassword = "12345678";
  WiFi.softAP(apSsid, apPassword);
  Serial.print("🌱 AP elérhető: ");
  Serial.println(WiFi.softAPIP());

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

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("✅ STA IP-cím: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("\n⚠️ Nem sikerült csatlakozni a ház WiFi-hez.");
      }
    } else {
      Serial.println("⚠️ Hiba a wifi.json fájl beolvasásakor.");
    }
  } else {
    Serial.println("ℹ️ Nincs elmentett WiFi beállítás (wifi.json).");
  }
}

void handleWiFiScanRequest(AsyncWebServerRequest *request) {
  int n = WiFi.scanNetworks();
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < n; i++) {
    arr.add(WiFi.SSID(i));
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
