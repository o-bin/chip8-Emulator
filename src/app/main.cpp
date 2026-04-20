#include "chip8/Chip8VM.hpp"
#include "chip8/Config.hpp"
#include "chip8/Rom.hpp"
#include "chip8/SdlPlatform.hpp"
#include "chip8/InputBindingsStore.hpp"

#include <SDL.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <sys/types.h>

#include "ui/BitmapFont.hpp"
#include "ui/RomBrowser.hpp"

namespace {

void PrintUsage(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " <rom_path> [--scale N] [--window WxH] [--reserve-right PX]"
            << " [--margin PX]"
            << " [--cpu-hz N] [--seed N] [--max-ms N] [--headless]\n";
}

std::string OpenZenityFileDialog() {
  std::string temp_file = "/tmp/zenity_result_" + std::to_string(getpid()) + ".txt";
  std::string command = "zenity --file-selection --title='Select ROM File' --file-filter='ROM files | *.ch8 *.rom *.bin' --file-filter='All files | *' > " + temp_file;
  
  // Hide SDL window while dialog is open
  SDL_HideWindow(SDL_GL_GetCurrentWindow());
  
  int result = system(command.c_str());
  
  // Show SDL window again
  SDL_ShowWindow(SDL_GL_GetCurrentWindow());
  
  if (result == 0) {
    // Read the result from temp file
    std::ifstream file(temp_file);
    if (file.is_open()) {
      std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      file.close();
      // Remove trailing newline
      if (!content.empty() && content.back() == '\n') {
        content.pop_back();
      }
      // Clean up temp file
      std::remove(temp_file.c_str());
      return content;
    }
  }
  
  // Clean up temp file if it exists
  std::remove(temp_file.c_str());
  return "";
}

std::string RunRomPicker() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
  }

  SDL_Window* win = SDL_CreateWindow(
    "chip8-launcher",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    900,
    600,
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );
  if (!win) throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

  SDL_Renderer* r = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!r) r = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
  if (!r) throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

  chip8::ui::BitmapFont font{};
  chip8::ui::RomBrowser browser(std::filesystem::current_path());

  std::string selected;
  bool running = true;
  while (running && selected.empty()) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) running = false;
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
      browser.HandleEvent(e);
    }

    int ww = 0, wh = 0;
    SDL_GetWindowSize(win, &ww, &wh);
    const auto res = browser.Render(r, font, ww, wh);
    if (res.selected_rom) selected = res.selected_rom->string();
    if (res.action == chip8::ui::RomBrowserAction::OpenZenityDialog) {
      // Open Zenity file dialog
      std::string zenity_result = OpenZenityFileDialog();
      if (!zenity_result.empty()) {
        selected = zenity_result;
        break;
      }
      // If dialog was cancelled, just continue with the browser
    }
    if (res.action == chip8::ui::RomBrowserAction::LaunchConfig) {
      // Close the browser window temporarily
      SDL_DestroyRenderer(r);
      SDL_DestroyWindow(win);
      
      // Launch the config tool
      const int result = std::system("./build/chip8_config");
      (void)result; // Ignore result for now
      
      // Recreate browser window
      win = SDL_CreateWindow(
        "chip8-launcher",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        900,
        600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
      );
      if (!win) throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

      r = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
      if (!r) r = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
      if (!r) throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
      
      continue; // Skip the delay and continue the loop
    }
    if (res.action == chip8::ui::RomBrowserAction::Quit) {
      selected.clear(); // Clear selection to trigger "No ROM selected"
      break;
    }
    SDL_Delay(8);
  }

  SDL_DestroyRenderer(r);
  SDL_DestroyWindow(win);
  // Do NOT SDL_Quit here; the emulator window will reuse SDL.

  if (selected.empty()) throw std::runtime_error("No ROM selected");
  return selected;
}

} // namespace

int main(int argc, char** argv) {
  try {
    std::string rom_path;
    chip8::Config cfg = chip8::DefaultConfig();
    std::uint32_t max_ms = 0;
    bool headless = false;

    // Optional CLI ROM path; otherwise user selects via browser UI.
    if (argc >= 2 && std::string(argv[1]).rfind("--", 0) != 0) {
      rom_path = argv[1];
    }

    const int arg_start = rom_path.empty() ? 1 : 2;
    for (int i = arg_start; i < argc; i++) {
      const std::string a = argv[i];
      auto need = [&](const char* name) -> const char* {
        if (i + 1 >= argc) throw std::runtime_error(std::string("Missing value for ") + name);
        return argv[++i];
      };

      if (a == "--scale") cfg.display.scale = static_cast<std::uint32_t>(std::stoul(need("--scale")));
      else if (a == "--window") {
        const std::string v = need("--window");
        const auto x = v.find('x');
        if (x == std::string::npos) throw std::runtime_error("Invalid --window, expected WxH");
        cfg.window.width = static_cast<std::uint32_t>(std::stoul(v.substr(0, x)));
        cfg.window.height = static_cast<std::uint32_t>(std::stoul(v.substr(x + 1)));
      }
      else if (a == "--reserve-right") cfg.window.reserved_right_px = static_cast<std::uint32_t>(std::stoul(need("--reserve-right")));
      else if (a == "--margin") cfg.window.margin_px = static_cast<std::uint32_t>(std::stoul(need("--margin")));
      else if (a == "--cpu-hz") cfg.timing.cpu_hz = static_cast<std::uint32_t>(std::stoul(need("--cpu-hz")));
      else if (a == "--seed") cfg.rng_seed = static_cast<std::uint32_t>(std::stoul(need("--seed")));
      else if (a == "--max-ms") max_ms = static_cast<std::uint32_t>(std::stoul(need("--max-ms")));
      else if (a == "--headless") headless = true;
      else throw std::runtime_error("Unknown arg: " + a);
    }

    chip8::Chip8VM vm(cfg);

    using clock = std::chrono::steady_clock;
    const auto cpu_step =
      std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0 / static_cast<double>(cfg.timing.cpu_hz)));
    const auto timer_step =
      std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0 / static_cast<double>(cfg.timing.timer_hz)));

    auto next_cpu = clock::now();
    auto next_timer = clock::now();
    const auto started = clock::now();

    bool running = true;
    if (headless) {
      // Run without creating a window/renderer (useful for CI or dummy video drivers).
      while (running) {
        const auto now = clock::now();
        while (now >= next_cpu) {
          vm.Step();
          next_cpu += cpu_step;
        }
        while (now >= next_timer) {
          vm.TickTimers();
          next_timer += timer_step;
        }

        if (max_ms != 0) {
          const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - started).count();
          if (elapsed >= static_cast<long long>(max_ms)) break;
        } else {
          // Avoid infinite loops in headless by requiring --max-ms
          break;
        }
      }
      return 0;
    }

    // Load persisted bindings (if present) and apply to platform.
    {
      chip8::InputBindingsStore store{cfg.input_bindings_path};
      cfg.input = chip8::LoadBindingsOrDefault(store, cfg.input);
    }

    if (rom_path.empty()) {
      rom_path = RunRomPicker();
    }

    auto load_rom = [&](const std::string& path) {
      chip8::Rom rom = chip8::LoadRomFromFile(path);
      vm.LoadRom(rom);
    };
    load_rom(rom_path);

    chip8::SdlPlatform platform(cfg);
    platform.SetBindings(cfg.input);

    while (running) {
      running = platform.PumpEvents(vm.state());

      const auto now = clock::now();
      while (now >= next_cpu) {
        vm.Step();
        next_cpu += cpu_step;
      }
      while (now >= next_timer) {
        vm.TickTimers();
        next_timer += timer_step;
      }

      if (vm.state().display.dirty) {
        platform.Present(vm.state());
        vm.state().display.dirty = false;
      }

      if (max_ms != 0) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - started).count();
        if (elapsed >= static_cast<long long>(max_ms)) break;
      }
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
}

