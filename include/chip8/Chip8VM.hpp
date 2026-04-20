#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

#include "chip8/Chip8State.hpp"
#include "chip8/Config.hpp"
#include "chip8/Rom.hpp"

namespace chip8 {

class Chip8VM {
public:
  explicit Chip8VM(Config cfg);

  const Config& config() const { return cfg_; }
  Chip8State& state() { return st_; }
  const Chip8State& state() const { return st_; }

  void Reset();
  void LoadRom(const Rom& rom);

  // Executes one instruction (or a blocking key-wait check)
  void Step();

  // Decrement timers at timer_hz
  void TickTimers();

private:
  Config cfg_;
  Chip8State st_{};
  std::mt19937 rng_;

  Word FetchOpcode() const;
  void Exec(Word op);

  void Op00E0();
  void Op00EE();
  void Op1NNN(Word op);
  void Op2NNN(Word op);
  void Op3XKK(Word op);
  void Op4XKK(Word op);
  void Op5XY0(Word op);
  void Op6XKK(Word op);
  void Op7XKK(Word op);
  void Op8XY_(Word op);
  void Op9XY0(Word op);
  void OpANNN(Word op);
  void OpBNNN(Word op);
  void OpCXKK(Word op);
  void OpDXYN(Word op);
  void OpEX__(Word op);
  void OpFX__(Word op);

  void Push(Word addr);
  Word Pop();

  void WriteFontset();
};

} // namespace chip8

