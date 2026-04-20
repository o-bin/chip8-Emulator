#pragma once

#include <cstdint>
#include <string_view>

struct SDL_Renderer;

namespace chip8::ui {

// Tiny 8x8 bitmap font renderer (ASCII subset).
class BitmapFont {
public:
  void DrawText(SDL_Renderer* r, int x, int y, std::string_view text, std::uint32_t scale = 1) const;
  int MeasureWidth(std::string_view text, std::uint32_t scale = 1) const;
  int LineHeight(std::uint32_t scale = 1) const { return static_cast<int>(8 * scale); }

private:
  const std::uint8_t* Glyph(char c) const;
};

} // namespace chip8::ui

