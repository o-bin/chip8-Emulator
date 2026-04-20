#pragma once

#include <array>
#include <cstdint>

namespace chip8 {

using Byte = std::uint8_t;
using Word = std::uint16_t;

struct Address {
  Word value{0};
};

struct Key {
  Byte value{0}; // 0x0..0xF
};

enum class Variant {
  Chip8,
  Schip11,
};

} // namespace chip8

