#pragma once

#include <cstdint>

// One step of a 32-bit xorshift generator; a zero state never moves, so seed it non-zero.
inline uint32_t xorshift32(uint32_t& state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}
