#include "KOReaderCredentialStore.h"

#include <Logging.h>
#include <MD5Builder.h>
#include <ObfuscationUtils.h>

namespace {
// The one sync server the firmware talks to. Custom servers are not supported.
constexpr char DEFAULT_SERVER_URL[] = "https://sync.crosspointreader.com";

// Bumped when a change to defaults would alter behavior for existing configs.
constexpr uint8_t CONFIG_VERSION = 2;
}  // namespace

void KOReaderCredentialStore::toJson(JsonDocument& doc) const {
  doc["cfgVersion"] = CONFIG_VERSION;
  // Serialize fields directly: public getters lazy-load and saveToFile() already
  // holds the store mutex. Calling a getter here could recursively load while locked.
  doc["username"] = username;
  doc["password_obf"] = obfuscation::obfuscateToBase64(password);
  doc["matchMethod"] = static_cast<uint8_t>(matchMethod);
  doc["sendMetadata"] = sendMetadata;
  doc["syncBehavior"] = static_cast<uint8_t>(syncBehavior);
}

bool KOReaderCredentialStore::fromJson(JsonVariantConst doc) {
  std::string user = doc["username"] | "";

  bool needsResave = false;
  obfuscation::DecodeStatus status = obfuscation::DecodeStatus::INVALID;
  std::string pass = obfuscation::deobfuscateFromBase64(doc["password_obf"] | "", &status);
  if (status == obfuscation::DecodeStatus::LEGACY && !pass.empty()) {
    needsResave = true;
  }
  if (status == obfuscation::DecodeStatus::INVALID || status == obfuscation::DecodeStatus::EMPTY || pass.empty()) {
    pass = doc["password"] | "";
    if (!pass.empty()) needsResave = true;
  }
  if (status == obfuscation::DecodeStatus::INVALID && pass.empty()) {
    LOG_ERR("KRS", "Ignoring unreadable KOReader password");
  }

  setCredentials(user, pass);

  return true;
}

void KOReaderCredentialStore::setCredentials(const std::string& user, const std::string& pass) {
  ensureLoaded();
  username = user;
  password = pass;
}

std::string KOReaderCredentialStore::getMd5Password() const {
  ensureLoaded();
  if (password.empty()) {
    return "";
  }

  // Calculate MD5 hash of password using ESP32's MD5Builder
  MD5Builder md5;
  md5.begin();
  md5.add(password.c_str());
  md5.calculate();

  return md5.toString().c_str();
}

bool KOReaderCredentialStore::hasCredentials() const {
  ensureLoaded();
  return !username.empty() && !password.empty();
}

std::string KOReaderCredentialStore::getBaseUrl() const {
  // The sync server is fixed. Custom servers are not supported.
  return DEFAULT_SERVER_URL;
}

bool KOReaderCredentialStore::usesCrossPointSyncServer() const { return getBaseUrl() == DEFAULT_SERVER_URL; }

void KOReaderCredentialStore::setMatchMethod(DocumentMatchMethod method) {
  ensureLoaded();
  matchMethod = method;
}

void KOReaderCredentialStore::setSendMetadata(bool enabled) {
  ensureLoaded();
  sendMetadata = enabled;
}

void KOReaderCredentialStore::setSyncBehavior(KOReaderSyncBehavior behavior) {
  ensureLoaded();
  if (static_cast<uint8_t>(behavior) > static_cast<uint8_t>(KOReaderSyncBehavior::SMART)) {
    behavior = KOReaderSyncBehavior::ASK_EVERY_TIME;
  }
  syncBehavior = behavior;
}
