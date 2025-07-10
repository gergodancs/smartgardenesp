#ifndef LOG_HELPER_H
#define LOG_HELPER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <time.h>

inline void appendToLog(const String &message) {
  File file = LittleFS.open("/log.txt", "a");
  if (!file) {
    Serial.println("❌ Nem sikerült megnyitni a log.txt fájlt.");
    return;
  }

  struct tm timeinfo;
  String prefix = "";
  if (getLocalTime(&timeinfo)) {
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    prefix = String(buffer);
  } else {
    prefix = "UNKNOWN TIME";
  }

  file.printf("[%s] %s\n", prefix.c_str(), message.c_str());
  file.close();
}
#endif
