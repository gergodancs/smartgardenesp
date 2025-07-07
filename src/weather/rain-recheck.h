#ifndef RAIN_RECHECK_H
#define RAIN_RECHECK_H

#include <vector>

struct SkippedZone {
  int zoneId;
  int maxMoisture;
};

extern std::vector<SkippedZone> rainSkippedZones;
extern const char* RAIN_SKIPPED_FILE;

void loadRainSkippedZones();
void saveRainSkippedZones();
void addRainSkippedZone(int zoneId, int maxMoisture);
void checkRainRecheckZones();

#endif
