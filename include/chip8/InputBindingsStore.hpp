#pragma once

#include <string>

#include "chip8/InputBindings.hpp"

namespace chip8 {

struct InputBindingsStore {
  std::string path; // e.g. "config/input_bindings.txt"
};

// Simple, dependency-free format:
//   host_key=chip8_hex
// Example:
//   w=1
//   up=1
//   s=4
//   down=4
InputBindings LoadBindingsOrDefault(const InputBindingsStore& store, const InputBindings& fallback);
void SaveBindings(const InputBindingsStore& store, const InputBindings& bindings);

} // namespace chip8

