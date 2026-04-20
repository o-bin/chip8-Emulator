#pragma once

#include <cstdint>
#include <string>

#include "chip8/Chip8VM.hpp"
#include "chip8/InputBindings.hpp"

namespace chip8 {

class SdlPlatform {
public:
  explicit SdlPlatform(const Config& cfg);
  ~SdlPlatform();

  SdlPlatform(const SdlPlatform&) = delete;
  SdlPlatform& operator=(const SdlPlatform&) = delete;

  // returns false when user requests quit
  bool PumpEvents(Chip8State& st);
  void Present(const Chip8State& st);

  // Allows rebinding keys at runtime (e.g. via config UI overlay).
  void SetBindings(const InputBindings& bindings);

private:
  struct Impl;
  Impl* impl_{nullptr};
};

} // namespace chip8

