#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "chip8/Chip8Types.hpp"

namespace chip8 {

struct Rom {
  std::string name;
  std::vector<Byte> bytes;
};

Rom LoadRomFromFile(const std::string& path);

} // namespace chip8

