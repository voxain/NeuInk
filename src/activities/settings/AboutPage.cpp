#include "AboutPage.h"

#include <Arduino.h>
#include <BoardConfig.h>
#include <HalPowerManager.h>
#include <I18n.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>
#include <memory>

#include "CrossPointSettings.h"
#include "RowListActivity.h"
#include "activities/reader/BookReadingStats.h"
#include "activities/reader/GlobalReadingStats.h"
#include "components/UITheme.h"

namespace {

using Row = RowListActivity::Row;

std::string megabytes(const uint64_t bytes) {
  char text[24];
  snprintf(text, sizeof(text), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
  return text;
}

// "used / total" for a memory.
std::string usedOf(const uint64_t used, const uint64_t total) {
  char text[48];
  snprintf(text, sizeof(text), "%.1f / %.1f MB", static_cast<double>(used) / (1024.0 * 1024.0),
           static_cast<double>(total) / (1024.0 * 1024.0));
  return text;
}

void header(std::vector<Row>& rows, const char* label) {
  Row row;
  row.kind = Row::Kind::Header;
  row.label = label;
  rows.push_back(std::move(row));
}

void info(std::vector<Row>& rows, const char* label, std::string value) {
  Row row;
  row.label = label;
  row.value = std::move(value);
  rows.push_back(std::move(row));
}

std::string duration(const uint32_t seconds) {
  char text[48];
  BookReadingStats::formatDuration(seconds, text, sizeof(text));
  return text;
}

}  // namespace

std::unique_ptr<Activity> makeAboutPage(GfxRenderer& renderer, MappedInputManager& mappedInput) {
  return std::make_unique<LambdaRowList>(
      renderer, mappedInput, "About", tr(STR_ABOUT), [](std::vector<Row>& rows, RowListActivity&) {
        rows.clear();

        header(rows, tr(STR_ABOUT_SOFTWARE));
        info(rows, tr(STR_ABOUT_VERSION), std::string(tr(STR_CROSSINK)) + " " + CROSSINK_VERSION);
        info(rows, tr(STR_ABOUT_BUILD),
             std::string(CROSSINK_GIT_SHA) + (strcmp(CROSSINK_GIT_DIRTY, "0") == 0 ? "" : "+"));
        info(rows, tr(STR_ABOUT_FIRMWARE_SIZE), megabytes(ESP.getSketchSize()));

        header(rows, tr(STR_ABOUT_DEVICE));
        info(rows, tr(STR_ABOUT_MODEL), BoardConfig::ACTIVE.name);
        info(rows, tr(STR_DEVICE_NAME), SETTINGS.getEffectiveDeviceName());
        char chip[64];
        snprintf(chip, sizeof(chip), "%s rev %d, %d x %d MHz", ESP.getChipModel(), static_cast<int>(ESP.getChipRevision()),
                 static_cast<int>(ESP.getChipCores()), static_cast<int>(ESP.getCpuFreqMHz()));
        info(rows, tr(STR_ABOUT_CHIP), chip);
        info(rows, tr(STR_ABOUT_WIFI_ADDRESS), WiFi.macAddress().c_str());
        char battery[24];
        snprintf(battery, sizeof(battery), "%u%%  %u mV", static_cast<unsigned>(powerManager.getBatteryPercentage()),
                 static_cast<unsigned>(powerManager.getBatteryMillivolts()));
        info(rows, tr(STR_ABOUT_BATTERY), battery);
        char uptime[24];
        const uint32_t seconds = millis() / 1000;
        snprintf(uptime, sizeof(uptime), "%u:%02u:%02u", static_cast<unsigned>(seconds / 3600),
                 static_cast<unsigned>(seconds / 60 % 60), static_cast<unsigned>(seconds % 60));
        info(rows, tr(STR_ABOUT_UPTIME), uptime);

        header(rows, tr(STR_ABOUT_STORAGE));
        info(rows, tr(STR_ABOUT_FLASH), megabytes(ESP.getFlashChipSize()));
        info(rows, tr(STR_ABOUT_MEMORY), usedOf(ESP.getHeapSize() - ESP.getFreeHeap(), ESP.getHeapSize()));
        if (ESP.getPsramSize() > 0) {
          info(rows, tr(STR_ABOUT_PSRAM), usedOf(ESP.getPsramSize() - ESP.getFreePsram(), ESP.getPsramSize()));
        }

        const GlobalReadingStats stats = GlobalReadingStats::load();
        header(rows, tr(STR_ABOUT_READING));
        info(rows, tr(STR_ABOUT_BOOKS_FINISHED), std::to_string(stats.completedBooks));
        info(rows, tr(STR_ABOUT_TIME_READ), duration(stats.totalReadingSeconds));
        info(rows, tr(STR_ABOUT_PAGES_TURNED), std::to_string(stats.totalPagesTurned));
      });
}
