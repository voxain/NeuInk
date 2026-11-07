#pragma once

#include <atomic>
#include <string>

#include "FirmwareTargets.h"

class OtaUpdater {
  firmware_targets::Target target = firmware_targets::Target::NEUINK;
  bool updateAvailable = false;
  std::string latestVersion;
  std::string otaUrl;
  std::string otaSha256;
  size_t otaSize = 0;
  size_t processedSize = 0;
  size_t totalSize = 0;

 public:
  using ProgressCallback = void (*)(void* ctx);

  enum OtaUpdaterError {
    OK = 0,
    NO_UPDATE,
    HTTP_ERROR,
    JSON_PARSE_ERROR,
    UPDATE_OLDER_ERROR,
    INTERNAL_UPDATE_ERROR,
    OOM_ERROR,
    CANCELLED_ERROR,
    HASH_MISMATCH_ERROR,
    WRONG_DEVICE_ERROR,
    SD_CARD_FULL_ERROR,
  };

  size_t getOtaSize() const { return otaSize; }

  // Progress work covers both staging and flashing so it never moves backwards.
  size_t getProcessedSize() const { return processedSize; }

  size_t getTotalSize() const { return totalSize; }

  // NEUINK is the normal update. Any other target is a cross-grade: it reads that
  // project's latest release and installs it whatever its version number is.
  explicit OtaUpdater(const firmware_targets::Target updateTarget = firmware_targets::Target::NEUINK)
      : target(updateTarget) {}
  firmware_targets::Target getTarget() const { return target; }
  bool isCrossGrade() const { return firmware_targets::isCrossGrade(target); }
  bool isUpdateNewer() const;
  // Whether `version` (a NeuInk release number) is newer than the firmware running.
  static bool isNewerThanInstalled(const char* version);
  const std::string& getLatestVersion() const;
  OtaUpdaterError checkForUpdate();
  OtaUpdaterError installUpdate(ProgressCallback onProgress = nullptr, void* ctx = nullptr,
                                std::atomic<bool>* cancelRequested = nullptr);
};
