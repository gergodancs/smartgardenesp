// weather.cpp
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "weather.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "weather.h"
#include "rain-recheck.h"
#include "pinConfig.h"
#include "../watering-logic/common.h"


int cachedRainChance = 0;

String getLocationString() {
    if (!LittleFS.exists("/location.json")) return "";
  
    File file = LittleFS.open("/location.json", "r");
    DynamicJsonDocument doc(256);
    deserializeJson(doc, file);
    file.close();
  
    String city = doc["city"] | "";
    String country = doc["country"] | "";
  
    if (city.length() == 0) return "";
    if (country.length() == 0) return city;
  
    return city + "," + country;
  }

  void fetchWeatherForecast() {
    String location = getLocationString();
    if (location == "") {
      Serial.println("⚠️ Nincs beállítva lokáció, nem lehet időjárást lekérni.");
      return;
    }
  
    String url = "http://api.weatherapi.com/v1/forecast.json?key=2acf3b0415d041fca8a132314211412&q=" + location + "&days=3";
    HTTPClient http;
    http.begin(url);
  
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(8192);
      deserializeJson(doc, payload);
  
      JsonArray forecastDays = doc["forecast"]["forecastday"];
      int maxChance = 0;
  
      for (JsonObject day : forecastDays) {
        int chance = day["day"]["daily_chance_of_rain"] | 0;
        if (chance > maxChance) {
          maxChance = chance;
        }
      }
  
      cachedRainChance = maxChance;
      Serial.printf("🌦️ Legnagyobb eső esély a következő napokban: %d%%\n", cachedRainChance);
    } else {
      Serial.printf("❌ Weather fetch failed: %d\n", httpCode);
    }
  
    http.end();
  }

  bool handleRainForecast(JsonObject weather, int zoneId, int moisture, int maxMoisture, int sensorPin, int relayPin) {
    bool enabled = weather["enabled"] | false;
    int threshold = weather["rainChanceThreshold"] | 0;
    int forecastDays = weather["forecastDays"] | 1;
    int critical = weather["criticalMoisture"] | 0;
    int preFill = weather["preRainFill"] | 0;
  
    if (!enabled || cachedRainChance < threshold) return false;
  
    if (moisture >= critical) {
      Serial.printf("🌧️ Zóna %d kihagyva – %d%% esély esőre (%d napon belül), nedvesség %d%% ≥ kritikus %d%%\n",
        zoneId, cachedRainChance, forecastDays, moisture, critical);
      addRainSkippedZone(zoneId, maxMoisture);
      return true;
    }
  
    if (moisture < critical && preFill > 0 && moisture < preFill) {
      Serial.printf("🌧️ Zóna %d túl száraz (%d%% < %d%%), előlocsolás indul %d%%-ig\n",
        zoneId, moisture, critical, preFill);
      digitalWrite(relayPin, LOW);
      digitalWrite(RELAY_PUMP, LOW);
      activeZones.push_back(WateringZone{zoneId, relayPin, sensorPin, preFill});
      return true;
    }
  
    Serial.printf("🌧️ Zóna %d már elérte a preFill szintet, kihagyva\n", zoneId);
    addRainSkippedZone(zoneId, maxMoisture);
    return true;
  }
  
  