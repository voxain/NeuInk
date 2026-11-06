#pragma once

// Internet clock sync: time from NTP. Needs Wi-Fi. The time zone and summer
// time are set by the user in System > Device (no location lookup).
namespace clocksync {

// Sets the clock now. False when the time could not be fetched.
bool syncNow();

// Called after every Wi-Fi connect; re-syncs at most once an hour per boot.
void onWifiConnected();

}  // namespace clocksync
