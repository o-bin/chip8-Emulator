#include "chip8/Chip8VM.hpp"

#include <algorithm>
#include <stdexcept>

namespace chip8 {

namespace {
constexpr Byte kFont5x4[16 * 5] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
} // namespace

Chip8VM::Chip8VM(Config cfg)
  : cfg_(cfg)
  , rng_(cfg.rng_seed ? *cfg.rng_seed : std::random_device{}()) {
  Reset();
}

void Chip8VM::Reset() {
  st_ = Chip8State{};
  st_.PC = cfg_.memory.program_start.value;
  st_.display.SetMode(64, 32);
  WriteFontset();
}

void Chip8VM::WriteFontset() {
  const auto start = cfg_.memory.font_start.value;
  if (start + sizeof(kFont5x4) > st_.memory.size()) {
    throw std::runtime_error("Font start out of bounds");
  }
  std::copy(std::begin(kFont5x4), std::end(kFont5x4), st_.memory.begin() + start);
}

void Chip8VM::LoadRom(const Rom& rom) {
  Reset();
  const auto start = cfg_.memory.program_start.value;
  if (start + rom.bytes.size() > st_.memory.size()) {
    throw std::runtime_error("ROM too large for memory map");
  }
  std::copy(rom.bytes.begin(), rom.bytes.end(), st_.memory.begin() + start);
}

Word Chip8VM::FetchOpcode() const {
  const Word pc = st_.PC;
  return static_cast<Word>((st_.memory[pc] << 8) | st_.memory[pc + 1]);
}

void Chip8VM::Step() {
  if (st_.waiting_for_key) {
    for (Byte k = 0; k < 16; k++) {
      if (st_.keypad.IsDown(k)) {
        st_.V[st_.wait_reg_x] = k;
        st_.waiting_for_key = false;
        st_.PC += 2;
        break;
      }
    }
    return;
  }

  const Word op = FetchOpcode();
  st_.PC += 2;
  Exec(op);
}

void Chip8VM::TickTimers() {
  if (st_.DT > 0) st_.DT--;
  if (st_.ST > 0) st_.ST--;
  st_.beeper.on = (st_.ST > 0);
}

void Chip8VM::Exec(Word op) {
  switch (op & 0xF000) {
    case 0x0000:
      if (op == 0x00E0) return Op00E0();
      if (op == 0x00EE) return Op00EE();
      return; // 0NNN SYS ignored (legacy)
    case 0x1000: return Op1NNN(op);
    case 0x2000: return Op2NNN(op);
    case 0x3000: return Op3XKK(op);
    case 0x4000: return Op4XKK(op);
    case 0x5000: return Op5XY0(op);
    case 0x6000: return Op6XKK(op);
    case 0x7000: return Op7XKK(op);
    case 0x8000: return Op8XY_(op);
    case 0x9000: return Op9XY0(op);
    case 0xA000: return OpANNN(op);
    case 0xB000: return OpBNNN(op);
    case 0xC000: return OpCXKK(op);
    case 0xD000: return OpDXYN(op);
    case 0xE000: return OpEX__(op);
    case 0xF000: return OpFX__(op);
    default: return;
  }
}

void Chip8VM::Push(Word addr) {
  if (st_.SP >= st_.stack.size()) throw std::runtime_error("Stack overflow");
  st_.stack[st_.SP++] = addr;
}

Word Chip8VM::Pop() {
  if (st_.SP == 0) throw std::runtime_error("Stack underflow");
  return st_.stack[--st_.SP];
}

void Chip8VM::Op00E0() { st_.display.Clear(); }

void Chip8VM::Op00EE() { st_.PC = Pop(); }

void Chip8VM::Op1NNN(Word op) { st_.PC = (op & 0x0FFF); }

void Chip8VM::Op2NNN(Word op) {
  Push(st_.PC);
  st_.PC = (op & 0x0FFF);
}

void Chip8VM::Op3XKK(Word op) {
  const Byte x = (op >> 8) & 0xF;
  const Byte kk = op & 0xFF;
  if (st_.V[x] == kk) st_.PC += 2;
}

void Chip8VM::Op4XKK(Word op) {
  const Byte x = (op >> 8) & 0xF;
  const Byte kk = op & 0xFF;
  if (st_.V[x] != kk) st_.PC += 2;
}

void Chip8VM::Op5XY0(Word op) {
  if ((op & 0x000F) != 0) return;
  const Byte x = (op >> 8) & 0xF;
  const Byte y = (op >> 4) & 0xF;
  if (st_.V[x] == st_.V[y]) st_.PC += 2;
}

void Chip8VM::Op6XKK(Word op) {
  const Byte x = (op >> 8) & 0xF;
  st_.V[x] = op & 0xFF;
}

void Chip8VM::Op7XKK(Word op) {
  const Byte x = (op >> 8) & 0xF;
  st_.V[x] = static_cast<Byte>(st_.V[x] + (op & 0xFF));
}

void Chip8VM::Op8XY_(Word op) {
  const Byte x = (op >> 8) & 0xF;
  const Byte y = (op >> 4) & 0xF;
  switch (op & 0x000F) {
    case 0x0: st_.V[x] = st_.V[y]; break;
    case 0x1: st_.V[x] = static_cast<Byte>(st_.V[x] | st_.V[y]); break;
    case 0x2: st_.V[x] = static_cast<Byte>(st_.V[x] & st_.V[y]); break;
    case 0x3: st_.V[x] = static_cast<Byte>(st_.V[x] ^ st_.V[y]); break;
    case 0x4: {
      const Word sum = static_cast<Word>(st_.V[x]) + static_cast<Word>(st_.V[y]);
      st_.V[0xF] = (sum > 0xFF) ? 1 : 0;
      st_.V[x] = static_cast<Byte>(sum & 0xFF);
      break;
    }
    case 0x5: {
      st_.V[0xF] = (st_.V[x] > st_.V[y]) ? 1 : 0;
      st_.V[x] = static_cast<Byte>(st_.V[x] - st_.V[y]);
      break;
    }
    case 0x6: {
      Byte v = cfg_.quirks.shift_uses_vy ? st_.V[y] : st_.V[x];
      st_.V[0xF] = static_cast<Byte>(v & 0x1);
      st_.V[x] = static_cast<Byte>(v >> 1);
      break;
    }
    case 0x7: {
      st_.V[0xF] = (st_.V[y] > st_.V[x]) ? 1 : 0;
      st_.V[x] = static_cast<Byte>(st_.V[y] - st_.V[x]);
      break;
    }
    case 0xE: {
      Byte v = cfg_.quirks.shift_uses_vy ? st_.V[y] : st_.V[x];
      st_.V[0xF] = static_cast<Byte>((v >> 7) & 0x1);
      st_.V[x] = static_cast<Byte>(v << 1);
      break;
    }
    default: break;
  }
}

void Chip8VM::Op9XY0(Word op) {
  if ((op & 0x000F) != 0) return;
  const Byte x = (op >> 8) & 0xF;
  const Byte y = (op >> 4) & 0xF;
  if (st_.V[x] != st_.V[y]) st_.PC += 2;
}

void Chip8VM::OpANNN(Word op) { st_.I = (op & 0x0FFF); }

void Chip8VM::OpBNNN(Word op) { st_.PC = static_cast<Word>((op & 0x0FFF) + st_.V[0]); }

void Chip8VM::OpCXKK(Word op) {
  const Byte x = (op >> 8) & 0xF;
  const Byte kk = op & 0xFF;
  std::uniform_int_distribution<int> dist(0, 255);
  st_.V[x] = static_cast<Byte>(dist(rng_) & kk);
}

void Chip8VM::OpDXYN(Word op) {
  const Byte x = (op >> 8) & 0xF;
  const Byte y = (op >> 4) & 0xF;
  const Byte n = op & 0xF;

  st_.V[0xF] = 0;

  const std::uint32_t w = st_.display.width;
  const std::uint32_t h = st_.display.height;
  std::uint32_t px = st_.V[x] % w;
  std::uint32_t py = st_.V[y] % h;

  for (Byte row = 0; row < n; row++) {
    const Byte sprite = st_.memory[st_.I + row];
    for (Byte bit = 0; bit < 8; bit++) {
      const Byte mask = static_cast<Byte>(0x80 >> bit);
      const Byte sprite_pixel = (sprite & mask) ? 1 : 0;
      if (!sprite_pixel) continue;

      std::uint32_t dx = px + bit;
      std::uint32_t dy = py + row;
      if (cfg_.quirks.draw_wrap) {
        dx %= w;
        dy %= h;
      } else {
        if (dx >= w || dy >= h) continue;
      }

      const auto cur = st_.display.Get(dx, dy);
      const auto next = static_cast<Byte>(cur ^ 1);
      if (cur == 1 && next == 0) st_.V[0xF] = 1;
      st_.display.Set(dx, dy, next);
    }
  }
  st_.display.dirty = true;
}

void Chip8VM::OpEX__(Word op) {
  const Byte x = (op >> 8) & 0xF;
  const Byte kk = op & 0xFF;
  const Byte key = st_.V[x] & 0xF;
  switch (kk) {
    case 0x9E:
      if (st_.keypad.IsDown(key)) st_.PC += 2;
      break;
    case 0xA1:
      if (!st_.keypad.IsDown(key)) st_.PC += 2;
      break;
    default: break;
  }
}

void Chip8VM::OpFX__(Word op) {
  const Byte x = (op >> 8) & 0xF;
  const Byte kk = op & 0xFF;
  switch (kk) {
    case 0x07: st_.V[x] = st_.DT; break;
    case 0x0A:
      st_.waiting_for_key = true;
      st_.wait_reg_x = x;
      break;
    case 0x15: st_.DT = st_.V[x]; break;
    case 0x18: st_.ST = st_.V[x]; break;
    case 0x1E: {
      const Word sum = static_cast<Word>(st_.I + st_.V[x]);
      st_.V[0xF] = (sum > 0x0FFF) ? 1 : 0;
      st_.I = static_cast<Word>(sum & 0x0FFF);
      break;
    }
    case 0x29:
      st_.I = static_cast<Word>(cfg_.memory.font_start.value + (st_.V[x] & 0xF) * 5);
      break;
    case 0x33: {
      Byte v = st_.V[x];
      st_.memory[st_.I + 2] = static_cast<Byte>(v % 10);
      v /= 10;
      st_.memory[st_.I + 1] = static_cast<Byte>(v % 10);
      v /= 10;
      st_.memory[st_.I] = static_cast<Byte>(v % 10);
      break;
    }
    case 0x55: {
      for (Byte i = 0; i <= x; i++) st_.memory[st_.I + i] = st_.V[i];
      if (cfg_.quirks.fx55_fx65_increment_i) st_.I = static_cast<Word>(st_.I + x + 1);
      break;
    }
    case 0x65: {
      for (Byte i = 0; i <= x; i++) st_.V[i] = st_.memory[st_.I + i];
      if (cfg_.quirks.fx55_fx65_increment_i) st_.I = static_cast<Word>(st_.I + x + 1);
      break;
    }
    default: break;
  }
}

} // namespace chip8

