#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

// A size for people: in MB, or in GB from 1 GB up ("12.3 MB", "29.7 GB").
inline std::string formatByteSize(const uint64_t bytes) {
  char text[24];
  if (bytes >= (1ull << 30)) {
    snprintf(text, sizeof(text), "%.1f GB", static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
  } else {
    snprintf(text, sizeof(text), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
  }
  return text;
}
