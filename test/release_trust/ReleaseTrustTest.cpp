#include <gtest/gtest.h>

#include <string>

#include "network/ReleaseTrust.h"

TEST(ReleaseTrust, AcceptsHttpsToPinnedHosts) {
  EXPECT_NE(releasetrust::rootFor("https://github.com/voxain/NeuInk/releases/download/0.1.0/firmware-x4-pro.bin"),
            nullptr);
  EXPECT_NE(releasetrust::rootFor("https://release-assets.githubusercontent.com/github-production-release-asset/1/2"),
            nullptr);
  EXPECT_NE(releasetrust::rootFor("https://objects.githubusercontent.com/x"), nullptr);
  EXPECT_NE(releasetrust::rootFor("https://github.com:443/x?y=1"), nullptr);
}

TEST(ReleaseTrust, UsesADifferentRootPerHostFamily) {
  EXPECT_NE(releasetrust::rootFor("https://github.com/x"), releasetrust::rootFor("https://objects.githubusercontent.com/x"));
}

TEST(ReleaseTrust, RefusesEverythingElse) {
  EXPECT_EQ(releasetrust::rootFor("http://github.com/x"), nullptr);
  EXPECT_EQ(releasetrust::rootFor("https://example.com/x"), nullptr);
  EXPECT_EQ(releasetrust::rootFor("https://github.com.evil.net/x"), nullptr);
  EXPECT_EQ(releasetrust::rootFor("https://evil.net/github.com"), nullptr);
  EXPECT_EQ(releasetrust::rootFor("https://api.github.com/repos"), nullptr);
  EXPECT_EQ(releasetrust::rootFor(""), nullptr);
}

TEST(ReleaseTrust, ReturnsCertificatePem) {
  const std::string root = releasetrust::rootFor("https://github.com/x");
  EXPECT_EQ(root.rfind("-----BEGIN CERTIFICATE-----", 0), 0u);
}
