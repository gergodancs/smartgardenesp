// weather.h
#pragma once
#include <ArduinoJson.h>

void fetchWeatherForecast();
bool handleRainForecast(JsonObject weather, int zoneId, int moisture, int maxMoisture, int sensorPin, int relayPin);

extern int cachedRainChance;

// UI-hoz szükséges előrejelzések
struct WeatherDay {
  String label;
  int rainChance;
};

extern WeatherDay forecastData[3];
