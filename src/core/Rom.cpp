#include "chip8/Rom.hpp"

#include <fstream>
#include <stdexcept>

namespace chip8 {

Rom LoadRomFromFile(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("Failed to open ROM: " + path);

  std::vector<Byte> bytes;
  f.seekg(0, std::ios::end);
  auto size = f.tellg();
  if (size < 0) throw std::runtime_error("Failed to stat ROM: " + path);
  bytes.resize(static_cast<std::size_t>(size));
  f.seekg(0, std::ios::beg);
  if (!bytes.empty()) {
    f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  if (!f && !bytes.empty()) throw std::runtime_error("Failed to read ROM: " + path);

  Rom rom{};
  rom.name = path;
  rom.bytes = std::move(bytes);
  return rom;
}

} // namespace chip8

