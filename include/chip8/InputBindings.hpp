#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace chip8 {

// Platform-agnostic: host keys are named strings (e.g. "w", "up", "space").
// The SDL adapter resolves these via SDL_GetKeyFromName.
struct KeyBinding {
  std::string host_key;
  std::uint8_t chip8_key{0}; // 0x0..0xF
};

struct InputBindings {
  std::vector<KeyBinding> bindings;
};

} // namespace chip8

