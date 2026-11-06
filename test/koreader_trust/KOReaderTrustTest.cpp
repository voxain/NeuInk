#include <gtest/gtest.h>

#include <string>

#include "KOReaderTrust.h"

TEST(KOReaderTrust, AcceptsHttpsToPinnedHost) {
  EXPECT_NE(koreader::trustRootFor("https://sync.crosspointreader.com"), nullptr);
  EXPECT_NE(koreader::trustRootFor("https://sync.crosspointreader.com/"), nullptr);
  EXPECT_NE(koreader::trustRootFor("https://sync.crosspointreader.com/users/auth"), nullptr);
  EXPECT_NE(koreader::trustRootFor("https://sync.crosspointreader.com:443/users/auth"), nullptr);
  EXPECT_NE(koreader::trustRootFor("https://sync.crosspointreader.com?x=1"), nullptr);
}

TEST(KOReaderTrust, RefusesEverythingElse) {
  EXPECT_EQ(koreader::trustRootFor("http://sync.crosspointreader.com/users/auth"), nullptr);
  EXPECT_EQ(koreader::trustRootFor("https://example.com/users/auth"), nullptr);
  EXPECT_EQ(koreader::trustRootFor("https://sync.crosspointreader.com.evil.net/"), nullptr);
  EXPECT_EQ(koreader::trustRootFor("https://sync.crosspointreader.comx/"), nullptr);
  EXPECT_EQ(koreader::trustRootFor("https://evil.net/?sync.crosspointreader.com"), nullptr);
  EXPECT_EQ(koreader::trustRootFor(""), nullptr);
}

TEST(KOReaderTrust, ReturnsCertificatePem) {
  const std::string root = koreader::trustRootFor("https://sync.crosspointreader.com");
  EXPECT_EQ(root.rfind("-----BEGIN CERTIFICATE-----", 0), 0u);
}
