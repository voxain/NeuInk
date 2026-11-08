#pragma once

#include <AsciiText.h>

#include <cstdint>
#include <cstring>
#include <string>

// Which project's releases the over-the-air updater reads.
//
// NEUINK is the normal update path. The other three are "cross-grades": the same
// update mechanism, pointed at the VARAGH, CrossInk or CrossPoint releases, so
// a user can leave NeuInk without a computer. All four publish their firmware
// as GitHub release assets, so the pinned GitHub roots in ReleaseTrust.h cover
// every one of them.
//
// This header has no hardware dependencies so the asset rules can be tested on
// the host.

namespace firmware_targets {

enum class Target : uint8_t {
  NEUINK = 0,
  VARAGH = 1,
  CROSSINK = 2,
  CROSSPOINT = 3,
};

inline constexpr uint32_t kTargetCount = 4;

inline constexpr char kNeuInkReleaseUrl[] = "https://api.github.com/repos/voxain/NeuInk/releases/latest";
inline constexpr char kVaraghReleaseUrl[] = "https://api.github.com/repos/PewPewzxc/VARAGH/releases/latest";
inline constexpr char kCrossInkReleaseUrl[] = "https://api.github.com/repos/uxjulia/crossink/releases/latest";
inline constexpr char kCrossPointReleaseUrl[] =
    "https://api.github.com/repos/crosspoint-reader/crosspoint-reader/releases/latest";

inline bool isCrossGrade(const Target target) { return target != Target::NEUINK; }

inline const char* displayName(const Target target) {
  switch (target) {
    case Target::VARAGH:
      return "VARAGH";
    case Target::CROSSINK:
      return "CrossInk";
    case Target::CROSSPOINT:
      return "CrossPoint";
    case Target::NEUINK:
      break;
  }
  return "NeuInk";
}

// `neuInkUrl` lets a build override the NeuInk URL (CROSSINK_OTA_RELEASE_URL).
inline const char* releaseUrl(const Target target, const char* neuInkUrl = kNeuInkReleaseUrl) {
  switch (target) {
    case Target::VARAGH:
      return kVaraghReleaseUrl;
    case Target::CROSSINK:
      return kCrossInkReleaseUrl;
    case Target::CROSSPOINT:
      return kCrossPointReleaseUrl;
    case Target::NEUINK:
      break;
  }
  return neuInkUrl;
}

// The target travels through the silent-restart snapshot as a small integer.
// Anything out of range decodes to NEUINK, the safe normal update.
inline uint32_t toPayload(const Target target) { return static_cast<uint32_t>(target); }

inline Target fromPayload(const uint32_t payload) {
  return payload < kTargetCount ? static_cast<Target>(payload) : Target::NEUINK;
}

namespace detail {

// "firmware-<device>.bin" or "firmware-<device>-<anything>.bin". NeuInk, VARAGH
// and CrossInk all name their assets this way (CrossInk adds "-v1.6.1").
inline bool matchesFirmwareStem(const std::string& stem, const char* assetName) {
  const std::string exact = stem + ".bin";
  if (exact == assetName) return true;
  const std::string withDash = stem + "-";
  return ascii::startsWith(assetName, withDash.c_str()) && ascii::endsWith(assetName, ".bin");
}

// CrossPoint's own device names: it calls the X4 Pro "x4pro" and the X4 Classic
// "x4c". Returns nullptr for a device CrossPoint publishes no image for.
inline const char* crossPointDeviceName(const std::string& deviceType) {
  if (deviceType == "x4-pro") return "x4pro";
  if (deviceType == "x4-classic") return "x4c";
  if (deviceType == "x3-x4") return "x3-x4";
  if (deviceType == "sticky") return "sticky";
  return nullptr;
}

}  // namespace detail

// True when `assetName` is the firmware image of `target` for this device.
// `deviceType` is the build's CROSSINK_FIRMWARE_DEVICE_TYPE ("x4-pro", ...);
// an empty string means a generic build.
inline bool matchesAsset(const Target target, const char* deviceType, const char* assetName) {
  if (assetName == nullptr) return false;
  const std::string device = deviceType == nullptr ? "" : deviceType;

  if (target != Target::CROSSPOINT) {
    const std::string stem = device.empty() ? "firmware" : "firmware-" + device;
    return detail::matchesFirmwareStem(stem, assetName);
  }

  const char* name = detail::crossPointDeviceName(device);
  if (name == nullptr) return false;
  // The combined X3/X4 image is plain "firmware.bin" in CrossPoint releases.
  if (device == "x3-x4" && std::strcmp(assetName, "firmware.bin") == 0) return true;
  if (std::string("firmware-") + name + ".bin" == assetName) return true;
  // Some devices only have the versioned name: crosspoint-<version>-<device>.bin.
  const std::string tail = std::string("-") + name + ".bin";
  return ascii::startsWith(assetName, "crosspoint-") && ascii::endsWith(assetName, tail.c_str());
}

}  // namespace firmware_targets
