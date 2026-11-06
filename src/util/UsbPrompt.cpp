#include "UsbPrompt.h"

#include <Arduino.h>

#include <cstdint>

#ifndef SIMULATOR
#include <esp_attr.h>
#else
#define RTC_NOINIT_ATTR
#endif

namespace {
constexpr uint32_t kMagic = 0x55535062;  // "USPb"
constexpr unsigned long kUnpluggedMs = 10000;  // out this long counts as unplugged

// Kept over the restart that leaving USB mode makes.
RTC_NOINIT_ATTR uint32_t suppressed;

bool wasConnected = false;
unsigned long lastSeenMs = 0;
}  // namespace

namespace UsbPrompt {

void suppress() { suppressed = kMagic; }

bool shouldPrompt(const bool hostConnected) {
  const unsigned long now = millis();
  if (hostConnected) {
    lastSeenMs = now;
    const bool rising = !wasConnected;
    wasConnected = true;
    return rising && suppressed != kMagic;
  }
  wasConnected = false;
  if (suppressed == kMagic && now - lastSeenMs >= kUnpluggedMs) suppressed = 0;
  return false;
}

}  // namespace UsbPrompt
