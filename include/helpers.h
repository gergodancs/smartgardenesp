#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>

// Beállításfájl elérési útja zónához
String getZoneFilename(int zoneId);

// Relé pin egy adott zónához
int getRelayPin(int zoneId);

// Szenzor pin egy adott zónához
int getSensorPin(int zoneId);

#endif
