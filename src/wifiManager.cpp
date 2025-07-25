#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

bool wifiConnecting = false;
String pendingSsid, pendingPassword;
bool wifiEventRegistered = false;

void registerWiFiEventHandler() {
  if (wifiEventRegistered) return;
  wifiEventRegistered = true;

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (!wifiConnecting) return;

    if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
      Serial.println("📶 STA connected (de még nincs IP).");
    }
    else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      Serial.print("✅ IP-cím: ");
      Serial.println(WiFi.localIP());

      // WiFi konfiguráció mentése
      DynamicJsonDocument wifiConfig(256);
      wifiConfig["ssid"] = pendingSsid;
      wifiConfig["password"] = pendingPassword;

      File file = LittleFS.open("/wifi.json", "w");
      if (file) {
        serializeJson(wifiConfig, file);
        file.close();
        Serial.println("💾 WiFi konfiguráció mentve.");
      } else {
        Serial.println("⚠️ Mentés sikertelen.");
      }

      wifiConnecting = false;
    }
    else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      Serial.printf("❌ STA kapcsolat megszakadt. Hiba: %d\n", info.wifi_sta_disconnected.reason);
      wifiConnecting = false;

      // Nincs leállítás vagy módváltás – AP végig aktív marad
    }
  });
}

void setupWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("SmartGarden", "12345678");
  Serial.print("🌱 AP elérhető: ");
  Serial.println(WiFi.softAPIP());

  bool staConnected = false;

  if (LittleFS.exists("/wifi.json")) {
    File file = LittleFS.open("/wifi.json", "r");
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, file) == DeserializationError::Ok) {
      String ssid = doc["ssid"];
      String password = doc["password"];
      file.close();

      WiFi.begin(ssid.c_str(), password.c_str());
      Serial.printf("📡 Csatlakozás a mentett hálózathoz: %s\n", ssid.c_str());

      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
      }

      staConnected = WiFi.status() == WL_CONNECTED;
    } else {
      Serial.println("⚠️ Érvénytelen wifi.json.");
      file.close();
    }
  } else {
    Serial.println("ℹ️ Nincs mentett WiFi konfiguráció.");
  }

  if (staConnected) {
    Serial.println("\n✅ STA csatlakozott");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    Serial.print("RSSI: "); Serial.println(WiFi.RSSI());
  } else {
    Serial.println("⚠️ Nem sikerült STA kapcsolat.");
  }
}

void handleWiFiScanRequest(AsyncWebServerRequest *request) {
  if (!LittleFS.exists("/wifi-scan.json")) {
    request->send(500, "application/json", R"({"error":"scan list not available"})");
    return;
  }

  File file = LittleFS.open("/wifi-scan.json", "r");
  request->send(file, "application/json");
  file.close();
}

void handleWiFiConnectRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (wifiConnecting) {
    request->send(400, "application/json", R"({"error":"Connection in progress"})");
    return;
  }

  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, data) != DeserializationError::Ok || !doc.containsKey("ssid") || !doc.containsKey("password")) {
    request->send(400, "application/json", R"({"error":"Invalid or missing SSID/password"})");
    return;
  }

  pendingSsid = doc["ssid"].as<String>();
  pendingPassword = doc["password"].as<String>();

  if (pendingSsid.isEmpty() || pendingSsid.length() > 32 || pendingPassword.length() > 64) {
    request->send(400, "application/json", R"({"error":"SSID or password invalid/too long"})");
    return;
  }

  wifiConnecting = true;
  Serial.printf("🔄 WiFi próbálkozás: %s\n", pendingSsid.c_str());

  // AP aktív marad – nincs softAPdisconnect()
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP_STA); // biztos, hogy dual módban marad
  WiFi.begin(pendingSsid.c_str(), pendingPassword.c_str());

  request->send(200, "application/json", R"({"status":"connecting"})");
}

#endif
