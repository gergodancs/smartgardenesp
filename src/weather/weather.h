// weather.h
#pragma once
void fetchWeatherForecast();
bool handleRainForecast(JsonObject weather, int zoneId, int moisture, int maxMoisture, int sensorPin, int relayPin);
extern int cachedRainChance; // pl. 0–100, vagy több napos tömb ha bonyolítod
