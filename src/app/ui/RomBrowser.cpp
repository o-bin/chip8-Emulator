#include "RomBrowser.hpp"

#include "BitmapFont.hpp"

#include <SDL.h>

#include <algorithm>
#include <iostream>

namespace chip8::ui {

RomBrowser::RomBrowser(std::filesystem::path start_dir) : dir_(std::move(start_dir)) {
  if (dir_.empty()) dir_ = std::filesystem::current_path();
  Refresh();
}

void RomBrowser::SetDir(const std::filesystem::path& p) {
  dir_ = p;
  Refresh();
}

bool RomBrowser::IsRomFile(const std::filesystem::path& p) const {
  const auto ext = p.extension().string();
  auto lower = ext;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return lower == ".ch8" || lower == ".rom" || lower == ".bin";
}

void RomBrowser::Refresh() {
  entries_.clear();
  selected_ = -1;
  scroll_ = 0;
  chosen_.reset();

  std::error_code ec;
  if (!std::filesystem::exists(dir_, ec) || !std::filesystem::is_directory(dir_, ec)) {
    dir_ = std::filesystem::current_path();
  }

  for (auto& e : std::filesystem::directory_iterator(dir_, ec)) {
    if (ec) break;
    if (e.is_directory() || (e.is_regular_file() && IsRomFile(e.path()))) entries_.push_back(e);
  }

  std::sort(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
    const bool ad = a.is_directory();
    const bool bd = b.is_directory();
    if (ad != bd) return ad > bd;
    return a.path().filename().string() < b.path().filename().string();
  });
}

bool RomBrowser::HandleEvent(const SDL_Event& e) {
  if (e.type == SDL_KEYDOWN) {
    if (menu_active_) {
      if (e.key.keysym.sym == SDLK_ESCAPE) {
        menu_active_ = false;
        return true;
      }
      if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_RIGHT || e.key.keysym.sym == SDLK_TAB) {
        menu_col_ = 1 - menu_col_;
        menu_sel_ = 0;
        return true;
      }
      if (e.key.keysym.sym == SDLK_UP) {
        if (menu_sel_ > 0) menu_sel_--;
        return true;
      }
      if (e.key.keysym.sym == SDLK_DOWN) {
        int max_sel = (menu_col_ == 0) ? 1 : 0;
        if (menu_sel_ < max_sel) menu_sel_++;
        return true;
      }
      if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        if (menu_col_ == 0) {
          if (menu_sel_ == 0) pending_action_ = RomBrowserAction::OpenZenityDialog; // Open ROM -> Zenity dialog
          else if (menu_sel_ == 1) pending_action_ = RomBrowserAction::Quit;
        } else {
          if (menu_sel_ == 0) pending_action_ = RomBrowserAction::LaunchConfig;
        }
        return true;
      }
      return true; // block other inputs while menu open
    }

    if (e.key.keysym.sym == SDLK_TAB) {
      menu_active_ = true;
      menu_col_ = 0;
      menu_sel_ = 0;
      return true;
    }
    if (e.key.keysym.sym == SDLK_F5) {
      Refresh();
      return true;
    }
    if (e.key.keysym.sym == SDLK_UP) {
      if (selected_ > 0) selected_--;
      return true;
    }
    if (e.key.keysym.sym == SDLK_DOWN) {
      if (selected_ + 1 < static_cast<int>(entries_.size())) selected_++;
      return true;
    }
    if (e.key.keysym.sym == SDLK_BACKSPACE) {
      if (dir_.has_parent_path()) SetDir(dir_.parent_path());
      return true;
    }
    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
      if (selected_ >= 0 && selected_ < static_cast<int>(entries_.size())) {
        const auto& ent = entries_[selected_];
        if (ent.is_directory()) SetDir(ent.path());
        else chosen_ = ent.path();
      }
      return true;
    }
  }
  if (e.type == SDL_MOUSEWHEEL) {
    if (menu_active_) return true;
    scroll_ -= e.wheel.y;
    if (scroll_ < 0) scroll_ = 0;
    return true;
  }
  if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
    int mx = e.button.x;
    int my = e.button.y;
    SDL_Rect file_rect{12, 10, 64, 16};
    SDL_Rect set_rect{108, 10, 128, 16};
    
    auto in_rect = [](int x, int y, SDL_Rect r) {
      return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
    };

    if (in_rect(mx, my, file_rect)) {
      if (menu_active_ && menu_col_ == 0) menu_active_ = false;
      else { menu_active_ = true; menu_col_ = 0; menu_sel_ = 0; }
      return true;
    }
    if (in_rect(mx, my, set_rect)) {
      if (menu_active_ && menu_col_ == 1) menu_active_ = false;
      else { menu_active_ = true; menu_col_ = 1; menu_sel_ = 0; }
      return true;
    }

    if (menu_active_) {
      int mw = (menu_col_ == 0) ? 120 : 200;
      int mx_base = (menu_col_ == 0) ? 12 : 108;
      int items = (menu_col_ == 0) ? 2 : 1;
      
      for (int i = 0; i < items; i++) {
        SDL_Rect item_rect{mx_base, 30 + i * 20, mw, 20};
        if (in_rect(mx, my, item_rect)) {
          menu_sel_ = i;
          if (menu_col_ == 0) {
            if (i == 0) pending_action_ = RomBrowserAction::OpenZenityDialog;
            else if (i == 1) pending_action_ = RomBrowserAction::Quit;
          } else {
            if (i == 0) pending_action_ = RomBrowserAction::LaunchConfig;
          }
          return true;
        }
      }
      
      menu_active_ = false;
      return true;
    }

    // Handle ROM file selection when menu is not active
    const int file_list_top = 70;
    const int row_height = 18;
    
    // Calculate which item was clicked based on mouse position
    if (my >= file_list_top) {
      int clicked_index = (my - file_list_top) / row_height + scroll_;
      if (clicked_index >= 0 && clicked_index < static_cast<int>(entries_.size())) {
        const auto& ent = entries_[clicked_index];
        if (ent.is_directory()) {
          SetDir(ent.path());
        } else {
          chosen_ = ent.path();
        }
        return true;
      }
    }

    return false;
  }
  return false;
}

RomBrowserResult RomBrowser::Render(SDL_Renderer* r, const BitmapFont& font, int win_w, int win_h) {
  RomBrowserResult out{};
  out.action = pending_action_;
  if (pending_action_ != RomBrowserAction::None) {
    pending_action_ = RomBrowserAction::None;
  }

  SDL_SetRenderDrawColor(r, 15, 15, 15, 255);
  SDL_RenderClear(r);

  // Draw Menu Headers
  if (menu_active_ && menu_col_ == 0) {
    SDL_Rect hl{10, 8, 68, 20};
    SDL_SetRenderDrawColor(r, 60, 60, 90, 255);
    SDL_RenderFillRect(r, &hl);
  } else if (menu_active_ && menu_col_ == 1) {
    SDL_Rect hl{106, 8, 132, 20};
    SDL_SetRenderDrawColor(r, 60, 60, 90, 255);
    SDL_RenderFillRect(r, &hl);
  }

  font.DrawText(r, 12, 10, "FILE  SETTINGS", 2);
  font.DrawText(r, 12, 40, dir_.string(), 1);
  font.DrawText(r, 12, win_h - 22, "ENTER=open  BACKSPACE=up  F5=refresh  ESC=quit", 1);

  const int top = 70;
  const int row_h = 18;
  const int visible = (win_h - top - 40) / row_h;
  const int start = std::min(scroll_, std::max(0, static_cast<int>(entries_.size()) - 1));

  for (int i = 0; i < visible; i++) {
    const int idx = start + i;
    if (idx >= static_cast<int>(entries_.size())) break;
    const auto& ent = entries_[idx];
    const bool sel = (idx == selected_);
    SDL_Rect rc{8, top + i * row_h - 2, win_w - 16, row_h};
    if (sel) {
      SDL_SetRenderDrawColor(r, 60, 60, 90, 255);
      SDL_RenderFillRect(r, &rc);
    }

    std::string label = ent.path().filename().string();
    if (ent.is_directory()) label = "[DIR] " + label;
    font.DrawText(r, 12, top + i * row_h, label, 1);
  }

  // Draw dropdown if active
  if (menu_active_) {
    int mx_base = (menu_col_ == 0) ? 12 : 108;
    int mw = (menu_col_ == 0) ? 120 : 200;
    int items = (menu_col_ == 0) ? 2 : 1;
    
    SDL_Rect bg{mx_base, 30, mw, items * 20 + 4};
    SDL_SetRenderDrawColor(r, 40, 40, 40, 255);
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &bg);

    const char* labels_0[] = {"Open ROM", "Quit"};
    const char* labels_1[] = {"Launch chip8_config"};
    const char** labels = (menu_col_ == 0) ? labels_0 : labels_1;

    for (int i = 0; i < items; i++) {
      SDL_Rect item_bg{mx_base + 2, 30 + 2 + i * 20, mw - 4, 20};
      if (menu_sel_ == i) {
        SDL_SetRenderDrawColor(r, 80, 80, 120, 255);
        SDL_RenderFillRect(r, &item_bg);
      }
      font.DrawText(r, mx_base + 8, 30 + 6 + i * 20, labels[i], 1);
    }
  }

  SDL_RenderPresent(r);

  if (chosen_) {
    out.selected_rom = *chosen_;
    chosen_.reset();
  }
  return out;
}

} // namespace chip8::ui

