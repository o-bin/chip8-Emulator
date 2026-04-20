#include "chip8/Config.hpp"
#include "chip8/InputBindingsStore.hpp"
#include "ui/BitmapFont.hpp"

#include <SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

struct UiState {
  std::optional<std::uint8_t> selected{};
  bool dirty{false};
};

static void Fill(SDL_Renderer* r, const SDL_Rect& rc, std::uint8_t rr, std::uint8_t gg, std::uint8_t bb) {
  SDL_SetRenderDrawColor(r, rr, gg, bb, 255);
  SDL_RenderFillRect(r, &rc);
}

static void Stroke(SDL_Renderer* r, const SDL_Rect& rc, std::uint8_t rr, std::uint8_t gg, std::uint8_t bb) {
  SDL_SetRenderDrawColor(r, rr, gg, bb, 255);
  SDL_RenderDrawRect(r, &rc);
}

static bool In(const SDL_Rect& r, int x, int y) {
  return x >= r.x && x < (r.x + r.w) && y >= r.y && y < (r.y + r.h);
}

static SDL_Rect SlotRect(int i) {
  const int col = i % 4;
  const int row = i / 4;
  const int x = 24 + col * 140;
  const int y = 24 + row * 90;
  return SDL_Rect{x, y, 128, 78};
}

static SDL_Rect SaveRect() { return SDL_Rect{24, 24 + 4 * 90 + 10, 140, 44}; }
static SDL_Rect ResetRect() { return SDL_Rect{24 + 140 + 14, 24 + 4 * 90 + 10, 140, 44}; }

static void SetBinding(chip8::InputBindings& b, std::uint8_t chip_key, const std::string& host_key) {
  b.bindings.erase(
    std::remove_if(b.bindings.begin(), b.bindings.end(), [&](const chip8::KeyBinding& kb) {
      return (kb.chip8_key & 0xF) == (chip_key & 0xF);
    }),
    b.bindings.end()
  );
  b.bindings.push_back(chip8::KeyBinding{host_key, static_cast<std::uint8_t>(chip_key & 0xF)});
}

} // namespace

int main(int argc, char** argv) {
  try {
    (void)argc;
    (void)argv;

    chip8::Config cfg = chip8::DefaultConfig();
    chip8::InputBindingsStore store{cfg.input_bindings_path};
    chip8::InputBindings bindings = chip8::LoadBindingsOrDefault(store, cfg.input);
    const chip8::InputBindings defaults = cfg.input;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
      throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    SDL_Window* win = SDL_CreateWindow(
      "chip8-config",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      620,
      450,
      SDL_WINDOW_SHOWN
    );
    if (!win) throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    SDL_Renderer* r = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!r) r = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!r) throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

    UiState ui{};
    bool running = true;
    while (running) {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
          const int mx = e.button.x;
          const int my = e.button.y;
          for (int i = 0; i < 16; i++) {
            const SDL_Rect slot = SlotRect(i);
            if (In(slot, mx, my)) ui.selected = static_cast<std::uint8_t>(i);
          }
          if (In(SaveRect(), mx, my)) {
            chip8::SaveBindings(store, bindings);
            ui.dirty = false;
          }
          if (In(ResetRect(), mx, my)) {
            bindings = defaults;
            ui.dirty = true;
          }
        }

        if (e.type == SDL_KEYDOWN) {
          if (e.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
          } else if (ui.selected.has_value()) {
            // Only assign keys when a slot is selected by mouse click
            const char* name = SDL_GetKeyName(e.key.keysym.sym);
            if (name && *name) {
              std::string s{name};
              if (s.size() == 1 && s[0] >= 'A' && s[0] <= 'Z') s[0] = static_cast<char>(s[0] - 'A' + 'a');
              SetBinding(bindings, *ui.selected, s);
              ui.dirty = true;
            }
          }
        }
      }

      chip8::ui::BitmapFont font{};

      SDL_SetRenderDrawColor(r, 15, 15, 15, 255);
      SDL_RenderClear(r);

      for (int i = 0; i < 16; i++) {
        const SDL_Rect slot = SlotRect(i);
        const bool selected = ui.selected.has_value() && *ui.selected == i;
        Fill(r, slot, selected ? 60 : 30, selected ? 60 : 30, selected ? 90 : 30);
        Stroke(r, slot, 220, 220, 220);
        
        // Draw CHIP-8 key number
        char key_label[8];
        snprintf(key_label, sizeof(key_label), "x%X", i);
        font.DrawText(r, slot.x + 4, slot.y + 4, key_label, 1);
        
        // Draw current binding
        std::string binding = "none";
        for (const auto& b : bindings.bindings) {
          if ((b.chip8_key & 0xF) == i) {
            binding = b.host_key;
            break;
          }
        }
        font.DrawText(r, slot.x + 4, slot.y + 20, binding, 1);
      }

      const SDL_Rect save = SaveRect();
      Fill(r, save, ui.dirty ? 40 : 25, ui.dirty ? 120 : 80, ui.dirty ? 40 : 25);
      Stroke(r, save, 220, 220, 220);
      font.DrawText(r, save.x + 8, save.y + 14, "Save", 1);
      
      const SDL_Rect reset = ResetRect();
      Fill(r, reset, 120, 50, 40);
      Stroke(r, reset, 220, 220, 220);
      font.DrawText(r, reset.x + 8, reset.y + 14, "Reset", 1);
      
      // Draw instructions
      font.DrawText(r, 24, 420, "Click a key slot, then press a key to bind. ESC to quit.", 1);

      SDL_RenderPresent(r);
      SDL_Delay(10);
    }

    // Auto-save if user changed something and didn't click Save.
    if (ui.dirty) chip8::SaveBindings(store, bindings);

    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
}

