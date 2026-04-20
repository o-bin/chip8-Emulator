#pragma once

#include <array>
#include <cstdint>

#include "chip8/Chip8Types.hpp"
#include "chip8/Devices.hpp"

namespace chip8 {

struct Chip8State {
  std::array<Byte, 4096> memory{};
  std::array<Byte, 16> V{};
  Word I{0};
  Word PC{0};
  std::array<Word, 16> stack{};
  Byte SP{0};
  Byte DT{0};
  Byte ST{0};

  Display display{};
  Keypad keypad{};
  Beeper beeper{};

  bool waiting_for_key{false};
  Byte wait_reg_x{0};
};

} // namespace chip8

