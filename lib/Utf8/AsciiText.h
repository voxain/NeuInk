#pragma once

#include <string_view>

// ASCII-only text comparisons: no locale, no allocation.
namespace ascii {

inline char lower(const char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

inline bool equalsIgnoreCase(const std::string_view a, const std::string_view b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (lower(a[i]) != lower(b[i])) return false;
  }
  return true;
}

inline bool startsWith(const std::string_view value, const std::string_view prefix) {
  return value.substr(0, prefix.size()) == prefix;
}

inline bool endsWith(const std::string_view value, const std::string_view suffix) {
  return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

inline bool endsWithIgnoreCase(const std::string_view value, const std::string_view suffix) {
  return value.size() >= suffix.size() && equalsIgnoreCase(value.substr(value.size() - suffix.size()), suffix);
}

}  // namespace ascii
