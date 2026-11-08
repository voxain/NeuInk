#pragma once

#include <cstdint>

// Little-endian reads and writes on a byte buffer, at an optional byte offset.
namespace byteorder {

inline uint16_t readLe16(const uint8_t* data, const int offset = 0) {
  return static_cast<uint16_t>(data[offset] | (data[offset + 1] << 8));
}

inline uint32_t readLe32(const uint8_t* data, const int offset = 0) {
  return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8) |
         (static_cast<uint32_t>(data[offset + 2]) << 16) | (static_cast<uint32_t>(data[offset + 3]) << 24);
}

inline void writeLe16(uint8_t* data, const int offset, const uint16_t value) {
  data[offset] = value & 0xFF;
  data[offset + 1] = (value >> 8) & 0xFF;
}

inline void writeLe32(uint8_t* data, const int offset, const uint32_t value) {
  writeLe16(data, offset, value & 0xFFFF);
  writeLe16(data, offset + 2, value >> 16);
}

}  // namespace byteorder
