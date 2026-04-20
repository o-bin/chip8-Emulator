#include "chip8/Config.hpp"

namespace chip8 {

Config DefaultConfig() {
  Config cfg{};
  cfg.variant = Variant::Chip8;

  // Conservative defaults; can be overridden by CLI later.
  cfg.quirks.shift_uses_vy = false;
  cfg.quirks.fx55_fx65_increment_i = false;
  cfg.quirks.draw_wrap = true;

  cfg.timing.cpu_hz = 700;
  cfg.timing.timer_hz = 60;

  cfg.memory.program_start = Address{0x200};
  cfg.memory.font_start = Address{0x050};

  cfg.display.scale = 10;

  // Window defaults: if 0, window derives from display scale (classic behavior).
  cfg.window.width = 0;
  cfg.window.height = 0;
  cfg.window.reserved_right_px = 0;
  cfg.window.margin_px = 0;
  cfg.window.keep_aspect = true;

  // No default key bindings - controls must be configured via chip8_config
  // or loaded from config/input_bindings.txt
  cfg.input.bindings = {};

  return cfg;
}

} // namespace chip8

