#pragma once

#include "util/BatteryHistory.h"

#if NEUINK_BATTERY_HISTORY
// The battery graph and the time and charge per app live in RTC memory, which survives sleep and software resets
// (an OTA or SD update, a Wi-Fi session's restart) but not a power-off or a flash through the cable. A copy goes to a small file on the SD card just before the reader sleeps (the one moment the card is free
// and nothing else is running), and comes back at boot when RTC memory is empty. Gaps heal themselves: what the time
// away covers is emptied as usual. Not saved: the minutes since the last sleep, if the battery runs flat first.
namespace UsageSnapshot {
void save();
void restore();
}  // namespace UsageSnapshot
#endif
