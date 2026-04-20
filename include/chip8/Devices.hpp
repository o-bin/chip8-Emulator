#pragma once

#include <array>
#include <cstdint>

namespace chip8 {

struct Display {
  std::uint32_t width{64};
  std::uint32_t height{32};
  std::array<std::uint8_t, 128 * 64> pixels{}; // 0/1, max size for SCHIP
  bool dirty{true};

  void SetMode(std::uint32_t w, std::uint32_t h) {
    width = w;
    height = h;
    Clear();
  }

  void Clear() {
    pixels.fill(0);
    dirty = true;
  }

  std::uint8_t Get(std::uint32_t x, std::uint32_t y) const {
    return pixels[y * 128 + x];
  }

  void Set(std::uint32_t x, std::uint32_t y, std::uint8_t v) {
    pixels[y * 128 + x] = v;
  }
};

struct Keypad {
  std::array<bool, 16> down{};
  bool IsDown(std::uint8_t k) const { return down[k & 0xF]; }
};

struct Beeper {
  bool on{false};
};

} // namespace chip8

