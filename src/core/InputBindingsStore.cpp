#include "chip8/InputBindingsStore.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace chip8 {

static std::string Trim(std::string s) {
  const auto is_ws = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
  while (!s.empty() && is_ws(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
  while (!s.empty() && is_ws(static_cast<unsigned char>(s.back()))) s.pop_back();
  return s;
}

InputBindings LoadBindingsOrDefault(const InputBindingsStore& store, const InputBindings& fallback) {
  std::ifstream f(store.path);
  if (!f) return fallback;

  InputBindings out{};
  std::string line;
  while (std::getline(f, line)) {
    line = Trim(line);
    if (line.empty() || line.starts_with("#")) continue;
    const auto eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string host = Trim(line.substr(0, eq));
    const std::string val = Trim(line.substr(eq + 1));
    if (host.empty() || val.empty()) continue;

    std::uint32_t chip = 0;
    std::istringstream iss(val);
    iss >> std::hex >> chip;
    if (!iss) continue;
    out.bindings.push_back(KeyBinding{host, static_cast<std::uint8_t>(chip & 0xF)});
  }

  return out.bindings.empty() ? fallback : out;
}

void SaveBindings(const InputBindingsStore& store, const InputBindings& bindings) {
  std::filesystem::path p(store.path);
  if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());

  std::ofstream f(store.path, std::ios::trunc);
  if (!f) throw std::runtime_error("Failed to write bindings: " + store.path);

  f << "# host_key=chip8_hex\n";
  for (const auto& b : bindings.bindings) {
    f << b.host_key << "=" << std::hex << static_cast<int>(b.chip8_key & 0xF) << std::dec << "\n";
  }
}

} // namespace chip8

