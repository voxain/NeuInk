#pragma once

#include <cstdint>

// What fills the SD card, for Settings > Storage. A scan walks every folder once, adds each file's size to a
// category, then reads the card's allocation table for how much is used in all (FAT32 keeps no count of its free
// clusters, so that is the slow part); what the files do not account for (folder entries, slack at the end of each
// file's last cluster, files of no known kind) is "Other". It runs on a task of its own and ends by itself, also
// when the page that asked for it is gone, so nothing can be waiting on it.
namespace StorageScan {

enum Category : uint8_t {
  Books,       // epub, xtc, txt, md
  Images,      // bmp, png, jpg, gif: pictures and screenshots
  Resources,   // SD fonts and dictionaries
  AppData,     // the book caches, covers and saved state in /.crosspoint
  Other,       // the rest of what is used
  kCategories
};

struct Result {
  uint64_t bytes[kCategories] = {};
  uint64_t used = 0;
  uint64_t total = 0;
  bool complete = false;  // false for a scan that was asked to end early
};

// Starts a scan unless one is running.
void start();
bool running();
// True once, after a scan has ended; its numbers are then in result().
bool takeFinished();
// Valid while !running().
const Result& result();
// Asks a running scan to end early. It does by itself soon; nothing needs to wait for it.
void stop();

}  // namespace StorageScan
