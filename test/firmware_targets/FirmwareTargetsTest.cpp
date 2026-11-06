#include <gtest/gtest.h>

#include "FirmwareTargets.h"

using namespace firmware_targets;

namespace {

TEST(FirmwareTargets, PayloadRoundTripsAndRejectsGarbage) {
  for (uint32_t i = 0; i < kTargetCount; ++i) {
    EXPECT_EQ(toPayload(fromPayload(i)), i);
  }
  // A corrupt snapshot must fall back to the normal NeuInk update.
  EXPECT_EQ(fromPayload(kTargetCount), Target::NEUINK);
  EXPECT_EQ(fromPayload(0xFFFFFFFFu), Target::NEUINK);
}

TEST(FirmwareTargets, OnlyNeuInkIsNotACrossGrade) {
  EXPECT_FALSE(isCrossGrade(Target::NEUINK));
  EXPECT_TRUE(isCrossGrade(Target::VARAGH));
  EXPECT_TRUE(isCrossGrade(Target::CROSSINK));
  EXPECT_TRUE(isCrossGrade(Target::CROSSPOINT));
}

TEST(FirmwareTargets, ReleaseUrlsAreTheUpstreamRepositories) {
  EXPECT_STREQ(releaseUrl(Target::NEUINK), "https://api.github.com/repos/voxain/NeuInk/releases/latest");
  EXPECT_STREQ(releaseUrl(Target::VARAGH), "https://api.github.com/repos/PewPewzxc/VARAGH/releases/latest");
  EXPECT_STREQ(releaseUrl(Target::CROSSINK), "https://api.github.com/repos/uxjulia/crossink/releases/latest");
  EXPECT_STREQ(releaseUrl(Target::CROSSPOINT),
               "https://api.github.com/repos/crosspoint-reader/crosspoint-reader/releases/latest");
  EXPECT_STREQ(releaseUrl(Target::NEUINK, "https://example.test/x"), "https://example.test/x");
  EXPECT_STREQ(releaseUrl(Target::VARAGH, "https://example.test/x"), kVaraghReleaseUrl);
}

// Asset lists copied from the real latest releases (Nov 2026).
TEST(FirmwareTargets, VaraghAssetsForX4Pro) {
  EXPECT_TRUE(matchesAsset(Target::VARAGH, "x4-pro", "firmware-x4-pro.bin"));
  EXPECT_FALSE(matchesAsset(Target::VARAGH, "x4-pro", "firmware-x4-classic.bin"));
  EXPECT_FALSE(matchesAsset(Target::VARAGH, "x4-pro", "firmware-x3-x4.bin"));
  EXPECT_FALSE(matchesAsset(Target::VARAGH, "x4-pro", "varagh-sd-card-v1.0.0.zip"));
  EXPECT_FALSE(matchesAsset(Target::VARAGH, "x4-pro", "varagh-v1.3.2-full-source.zip"));
}

TEST(FirmwareTargets, CrossInkAssetsCarryAVersionSuffix) {
  EXPECT_TRUE(matchesAsset(Target::CROSSINK, "x4-pro", "firmware-x4-pro-v1.6.1.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSINK, "x4-pro", "firmware-x4-classic-v1.6.1.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSINK, "x4-pro", "firmware-sticky-v1.6.1.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSINK, "x4-pro", "firmware-x3-x4-v1.6.1.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSINK, "x4-pro", "firmware-x4-pro-v1.6.1.zip"));
}

TEST(FirmwareTargets, CrossPointUsesItsOwnDeviceNames) {
  EXPECT_TRUE(matchesAsset(Target::CROSSPOINT, "x4-pro", "firmware-x4pro.bin"));
  EXPECT_TRUE(matchesAsset(Target::CROSSPOINT, "x4-pro", "crosspoint-2.0.0-x4pro.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "x4-pro", "firmware.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "x4-pro", "firmware-x4-pro.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "x4-pro", "firmware-sticky.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "x4-pro", "firmware-papermono.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "x4-pro", "crosspoint-2.0.0-x4c.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "x4-pro", "crosspoint-2.0.0-x3-x4.bin"));
}

TEST(FirmwareTargets, CrossPointOtherDevices) {
  EXPECT_TRUE(matchesAsset(Target::CROSSPOINT, "x4-classic", "crosspoint-2.0.0-x4c.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "x4-classic", "crosspoint-2.0.0-x4pro.bin"));
  EXPECT_TRUE(matchesAsset(Target::CROSSPOINT, "x3-x4", "firmware.bin"));
  EXPECT_TRUE(matchesAsset(Target::CROSSPOINT, "x3-x4", "crosspoint-2.0.0-x3-x4.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "x3-x4", "firmware-x4pro.bin"));
  EXPECT_TRUE(matchesAsset(Target::CROSSPOINT, "sticky", "firmware-sticky.bin"));
  // A device CrossPoint has no image for matches nothing, rather than a guess.
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "something-new", "firmware.bin"));
  EXPECT_FALSE(matchesAsset(Target::CROSSPOINT, "", "firmware.bin"));
}

TEST(FirmwareTargets, NeuInkKeepsItsOwnRule) {
  EXPECT_TRUE(matchesAsset(Target::NEUINK, "x4-pro", "firmware-x4-pro.bin"));
  EXPECT_FALSE(matchesAsset(Target::NEUINK, "x4-pro", "firmware-x4pro.bin"));
  EXPECT_TRUE(matchesAsset(Target::NEUINK, "", "firmware.bin"));
  EXPECT_FALSE(matchesAsset(Target::NEUINK, "x4-pro", nullptr));
}

}  // namespace
