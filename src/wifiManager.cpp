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
  if (!LittleFS.exists("/wifi-scan.json")) {
    request->send(500, "application/json", R"({"error":"scan list not available"})");
    return;
  }

  File file = LittleFS.open("/wifi-scan.json", "r");
  request->send(file, "application/json");
  file.close();
}




void handleWiFiConnectRequest(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  // 0. JSON parsing
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, data);

  if (error || !doc.containsKey("ssid") || !doc.containsKey("password")) {
    request->send(400, "application/json", R"({"error":"Missing or invalid JSON"})");
    return;
  }

  String ssid = doc["ssid"].as<String>();
  String password = doc["password"].as<String>();

  if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 64) {
    request->send(400, "application/json", R"({"error":"SSID or password missing/too long"})");
    return;
  }

  // 1. Meglévő STA kapcsolat bontása
  WiFi.disconnect(false);
  delay(100);
  WiFi.mode(WIFI_AP_STA);  // AP életben marad

  // 2. Új kapcsolat próbálkozás
  Serial.printf("🔄 Csatlakozás: %s\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());

  const unsigned long timeout = 10000;
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(100);
    yield();  // Watchdog védelem
    Serial.print(".");
  }
  Serial.println();

  DynamicJsonDocument response(256);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi kapcsolat sikeres");

    // 3. Csak sikeres kapcsolat után mentjük el a beállításokat
    DynamicJsonDocument wifiConfig(256);
    wifiConfig["ssid"] = ssid;
    wifiConfig["password"] = password;

    File file = LittleFS.open("/wifi.json", "w");
    if (!file) {
      response["status"] = "connected";
      response["ip"] = WiFi.localIP().toString();
      response["warning"] = "WiFi OK, but config file save failed";
    } else {
      serializeJson(wifiConfig, file);
      file.close();
      response["status"] = "connected";
      response["ip"] = WiFi.localIP().toString();
    }
  } else {
    Serial.println("❌ Csatlakozás sikertelen, törlés...");
    WiFi.disconnect(true);  // biztos bontás
    response["status"] = "failed";
    response["error"] = "Could not connect to WiFi with provided credentials";
  }

  String resStr;
  serializeJson(response, resStr);
  request->send(200, "application/json", resStr);
}
