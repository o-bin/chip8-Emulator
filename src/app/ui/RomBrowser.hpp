#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct SDL_Renderer;
union SDL_Event;

namespace chip8::ui {

class BitmapFont;

enum class RomBrowserAction {
  None,
  Quit,
  LaunchConfig,
  OpenZenityDialog
};

struct RomBrowserResult {
  std::optional<std::filesystem::path> selected_rom;
  RomBrowserAction action{RomBrowserAction::None};
};

class RomBrowser {
public:
  explicit RomBrowser(std::filesystem::path start_dir);

  const std::filesystem::path& current_dir() const { return dir_; }
  void SetDir(const std::filesystem::path& p);

  // Returns true if it consumed the event.
  bool HandleEvent(const SDL_Event& e);

  RomBrowserResult Render(SDL_Renderer* r, const BitmapFont& font, int win_w, int win_h);

private:
  std::filesystem::path dir_;
  std::vector<std::filesystem::directory_entry> entries_;
  int selected_{-1};
  int scroll_{0};
  std::optional<std::filesystem::path> chosen_{};

  bool menu_active_{false};
  int menu_col_{0}; // 0 = FILE, 1 = SETTINGS
  int menu_sel_{0};
  RomBrowserAction pending_action_{RomBrowserAction::None};

  void Refresh();
  bool IsRomFile(const std::filesystem::path& p) const;
};

} // namespace chip8::ui

