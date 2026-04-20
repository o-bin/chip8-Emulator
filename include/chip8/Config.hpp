#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "chip8/Chip8Types.hpp"
#include "chip8/InputBindings.hpp"

namespace chip8 {

struct Quirks {
  // Kept explicit + configurable; defaults set in Config.cpp
  bool shift_uses_vy{false};           // 8XY6/8XYE: use VY then store in VX
  bool fx55_fx65_increment_i{false};   // whether I changes after FX55/FX65
  bool draw_wrap{true};                // wrap vs clip for DRW
};

struct Timing {
  std::uint32_t cpu_hz{700};
  std::uint32_t timer_hz{60};
};

struct MemoryMap {
  Address program_start{0x200};
  Address font_start{0x050};
};

struct DisplayConfig {
  std::uint32_t scale{10};
};

struct WindowConfig {
  // If set to 0, defaults to display_w*scale (or display_h*scale)
  std::uint32_t width{0};
  std::uint32_t height{0};

  // Space reserved for future debug UI (e.g. ImGui panels)
  std::uint32_t reserved_right_px{0};

  // Extra padding around the render area (prevents sprites being "too close" to edges).
  std::uint32_t margin_px{0};

  bool keep_aspect{true};
};

struct Config {
  Variant variant{Variant::Chip8};
  Quirks quirks{};
  Timing timing{};
  MemoryMap memory{};
  DisplayConfig display{};
  WindowConfig window{};
  InputBindings input{};
  std::string input_bindings_path{"config/input_bindings.txt"};
  std::optional<std::uint32_t> rng_seed{};
};

Config DefaultConfig();

} // namespace chip8

