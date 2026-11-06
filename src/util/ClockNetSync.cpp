#include "ClockNetSync.h"

#include <Arduino.h>
#include <HalClock.h>

#include "CrossPointSettings.h"

namespace clocksync {

namespace {

constexpr unsigned long kResyncGapMs = 60UL * 60UL * 1000UL;
unsigned long lastSyncMs = 0;
bool syncedThisBoot = false;

}  // namespace

bool syncNow() {
  if (!halClock.syncFromNTP()) return false;
  SETTINGS.clockHasBeenSynced = 1;
  SETTINGS.clockDateHasBeenSynced = 1;
  lastSyncMs = millis();
  syncedThisBoot = true;
  SETTINGS.saveToFile();
  return true;
}

void onWifiConnected() {
  if (!halClock.isAvailable()) return;
  if (syncedThisBoot && millis() - lastSyncMs < kResyncGapMs) return;
  syncNow();
}

}  // namespace clocksync
