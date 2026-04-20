#include "chip8/SdlPlatform.hpp"

#include <SDL.h>

#include <array>
#include <unordered_map>
#include <stdexcept>

namespace chip8 {

struct SdlPlatform::Impl {
  SDL_Window* window{nullptr};
  SDL_Renderer* renderer{nullptr};
  SDL_Texture* texture{nullptr};
  std::uint32_t scale{10};
  std::uint32_t win_w{0};
  std::uint32_t win_h{0};
  std::uint32_t reserved_right_px{0};
  std::uint32_t margin_px{0};
  bool keep_aspect{true};
  std::uint32_t tex_w{64};
  std::uint32_t tex_h{32};

  std::array<std::uint32_t, 128 * 64> rgba{};

  std::unordered_map<SDL_Keycode, std::uint8_t> keymap{};
};

static std::unordered_map<SDL_Keycode, std::uint8_t> BuildKeymap(const InputBindings& bindings) {
  std::unordered_map<SDL_Keycode, std::uint8_t> out;
  out.reserve(bindings.bindings.size());

  for (const auto& b : bindings.bindings) {
    if (b.host_key.empty()) continue;
    const SDL_Keycode key = SDL_GetKeyFromName(b.host_key.c_str());
    if (key == SDLK_UNKNOWN) continue;
    out[key] = static_cast<std::uint8_t>(b.chip8_key & 0xF);
  }
  return out;
}

SdlPlatform::SdlPlatform(const Config& cfg) {
  impl_ = new Impl();
  impl_->scale = cfg.display.scale;
  impl_->win_w = cfg.window.width;
  impl_->win_h = cfg.window.height;
  impl_->reserved_right_px = cfg.window.reserved_right_px;
  impl_->margin_px = cfg.window.margin_px;
  impl_->keep_aspect = cfg.window.keep_aspect;
  impl_->keymap = BuildKeymap(cfg.input);

  // Keep audio out of the baseline to avoid host PulseAudio issues; add later as a separate adapter.
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
  }

  impl_->window = SDL_CreateWindow(
    "chip8",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    static_cast<int>(impl_->win_w ? impl_->win_w : (impl_->tex_w * impl_->scale)),
    static_cast<int>(impl_->win_h ? impl_->win_h : (impl_->tex_h * impl_->scale)),
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );
  if (!impl_->window) throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

  impl_->renderer = SDL_CreateRenderer(impl_->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!impl_->renderer) {
    impl_->renderer = SDL_CreateRenderer(impl_->window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (!impl_->renderer) {
    impl_->renderer = SDL_CreateRenderer(impl_->window, -1, 0);
  }
  if (!impl_->renderer) throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

  impl_->texture = SDL_CreateTexture(
    impl_->renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    static_cast<int>(impl_->tex_w),
    static_cast<int>(impl_->tex_h)
  );
  if (!impl_->texture) throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());
}

SdlPlatform::~SdlPlatform() {
  if (!impl_) return;
  if (impl_->texture) SDL_DestroyTexture(impl_->texture);
  if (impl_->renderer) SDL_DestroyRenderer(impl_->renderer);
  if (impl_->window) SDL_DestroyWindow(impl_->window);
  SDL_Quit();
  delete impl_;
  impl_ = nullptr;
}

bool SdlPlatform::PumpEvents(Chip8State& st) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) return false;
    if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
      if (e.key.keysym.sym == SDLK_ESCAPE && e.type == SDL_KEYDOWN) return false;
      auto it = impl_->keymap.find(e.key.keysym.sym);
      if (it != impl_->keymap.end()) st.keypad.down[it->second] = (e.type == SDL_KEYDOWN);
    }
  }
  return true;
}

void SdlPlatform::Present(const Chip8State& st) {
  const auto w = st.display.width;
  const auto h = st.display.height;

  if (w != impl_->tex_w || h != impl_->tex_h) {
    impl_->tex_w = w;
    impl_->tex_h = h;
    if (impl_->texture) SDL_DestroyTexture(impl_->texture);
    impl_->texture = SDL_CreateTexture(
      impl_->renderer,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,
      static_cast<int>(impl_->tex_w),
      static_cast<int>(impl_->tex_h)
    );
  }

  for (std::uint32_t y = 0; y < h; y++) {
    for (std::uint32_t x = 0; x < w; x++) {
      const auto p = st.display.Get(x, y) ? 0xFFFFFFFFu : 0xFF000000u;
      impl_->rgba[y * 128 + x] = p;
    }
  }

  SDL_UpdateTexture(impl_->texture, nullptr, impl_->rgba.data(), static_cast<int>(impl_->tex_w * sizeof(std::uint32_t)));

  SDL_SetRenderDrawColor(impl_->renderer, 0, 0, 0, 255);
  SDL_RenderClear(impl_->renderer);

  int win_w = 0, win_h = 0;
  SDL_GetWindowSize(impl_->window, &win_w, &win_h);

  // Reserve right-side space (future UI) and keep aspect ratio if enabled.
  const int usable_w = std::max(1, win_w - static_cast<int>(impl_->reserved_right_px) - static_cast<int>(impl_->margin_px * 2));
  const int usable_h = std::max(1, win_h - static_cast<int>(impl_->margin_px * 2));

  SDL_Rect dst{static_cast<int>(impl_->margin_px), static_cast<int>(impl_->margin_px), usable_w, usable_h};
  if (impl_->keep_aspect) {
    const double aspect = static_cast<double>(impl_->tex_w) / static_cast<double>(impl_->tex_h);
    int out_w = usable_w;
    int out_h = static_cast<int>(static_cast<double>(out_w) / aspect);
    if (out_h > usable_h) {
      out_h = usable_h;
      out_w = static_cast<int>(static_cast<double>(out_h) * aspect);
    }
    dst.w = std::max(1, out_w);
    dst.h = std::max(1, out_h);
    dst.x = static_cast<int>(impl_->margin_px) + (usable_w - dst.w) / 2;
    dst.y = static_cast<int>(impl_->margin_px) + (usable_h - dst.h) / 2;
  }

  SDL_RenderCopy(impl_->renderer, impl_->texture, nullptr, &dst);
  SDL_RenderPresent(impl_->renderer);
}

void SdlPlatform::SetBindings(const InputBindings& bindings) {
  impl_->keymap = BuildKeymap(bindings);
}

} // namespace chip8

