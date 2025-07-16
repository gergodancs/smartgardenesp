#ifndef LOG_HELPER_H
#define LOG_HELPER_H

#include <Arduino.h>
#include <LittleFS.h>

inline void appendToLog(const String &message) {
  File file = LittleFS.open("/log.txt", "a");
  if (!file) {
    Serial.println("❌ Nem sikerült megnyitni a log.txt fájlt.");
    return;
  }

  file.println(message);
  file.close();
}
#endif
