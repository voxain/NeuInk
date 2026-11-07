#include "StorageScan.h"

#include <HalStorage.h>
#include <Logging.h>

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

#include "FsHelpers.h"
#include "activities/apps/GameThinker.h"

namespace {

// The worker task and the numbers it fills in outlive any page. The scan runs on core 1, the one the screen is drawn
// from, which is idle between frames: the main loop and the touch input on core 0 keep their whole core.
GameThinker thinker;
StorageScan::Result scanResult;

bool equalsIgnoreCase(const char* a, const char* b) {
  for (; *a && *b; ++a, ++b) {
    if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) return false;
  }
  return *a == *b;
}

// A whole folder's kind where it decides it (the caches, the dictionaries); otherwise the file's own extension does.
enum class Folder : uint8_t { None, AppData, Resources };

Folder folderKind(const Folder parent, const bool atRoot, const char* name) {
  if (parent != Folder::None) return parent;
  if (!atRoot) return Folder::None;
  if (equalsIgnoreCase(name, ".crosspoint")) return Folder::AppData;
  if (equalsIgnoreCase(name, ".dictionaries") || equalsIgnoreCase(name, "dictionaries")) return Folder::Resources;
  return Folder::None;
}

bool category(const Folder folder, const char* fileName, StorageScan::Category& out) {
  const std::string_view name{fileName};
  if (folder == Folder::AppData) {
    out = StorageScan::AppData;
  } else if (folder == Folder::Resources || FsHelpers::checkFileExtension(name, ".cpfont")) {
    out = StorageScan::Resources;
  } else if (FsHelpers::hasEpubExtension(name) || FsHelpers::hasXtcExtension(name) || FsHelpers::hasTxtExtension(name) ||
             FsHelpers::hasMarkdownExtension(name)) {
    out = StorageScan::Books;
  } else if (FsHelpers::hasBmpExtension(name) || FsHelpers::hasPngExtension(name) || FsHelpers::hasJpgExtension(name) ||
             FsHelpers::hasGifExtension(name)) {
    out = StorageScan::Images;
  } else {
    return false;  // counted in "Other" once the card's used space is known
  }
  return true;
}

void scan(void*) {
  scanResult = StorageScan::Result{};
  scanResult.total = Storage.totalBytes();
  struct Pending {
    std::string path;
    Folder folder;
  };
  std::vector<Pending> pending;
  pending.push_back({"/", Folder::None});
  uint64_t counted = 0;
  char name[256];
  while (!pending.empty()) {
    const Pending dir = std::move(pending.back());
    pending.pop_back();
    HalFile folder = Storage.open(dir.path.c_str());
    if (!folder || !folder.isDirectory()) continue;
    const bool atRoot = dir.path == "/";
    for (HalFile entry = folder.openNextFile(); entry; entry = folder.openNextFile()) {
      if (thinker.shouldStop()) return;  // also gives the other tasks their turn
      entry.getName(name, sizeof(name));
      if (entry.isDirectory()) {
        if (atRoot && equalsIgnoreCase(name, "System Volume Information")) continue;
        pending.push_back({(atRoot ? "/" : dir.path + "/") + name, folderKind(dir.folder, atRoot, name)});
        continue;
      }
      StorageScan::Category kind;
      if (!category(dir.folder, name, kind)) continue;
      const uint64_t size = entry.fileSize64();
      scanResult.bytes[kind] += size;
      counted += size;
    }
  }
  scanResult.used = Storage.usedBytes();
  scanResult.bytes[StorageScan::Other] = scanResult.used > counted ? scanResult.used - counted : 0;
  scanResult.complete = true;
}

}  // namespace

namespace StorageScan {

void start() {
  if (!Storage.ready() || !thinker.start(&scan, nullptr, 0, portNUM_PROCESSORS > 1 ? 1 : tskNO_AFFINITY)) LOG_ERR("STORAGE", "Could not start the scan");
}
bool running() { return thinker.running(); }
bool takeFinished() { return thinker.takeFinished(); }
const Result& result() { return scanResult; }
void stop() { thinker.requestStop(); }

}  // namespace StorageScan
