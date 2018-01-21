#pragma once

#include "../gb/gb.hpp"

#include <SDL2/SDL.h>
#include <iostream>

class UiSDL
{
public:
  UiSDL(GB& gb, int scale, bool memory, bool tiles)
    : _gb(gb)
    , _scale(scale)
  {
    _init_sdl();

    _init_main();

    if (memory)
      _init_mem();

    if (tiles)
      _init_tile();
  }

  ~UiSDL()
  {
    SDL_DestroyRenderer(_tile_ren);
    SDL_DestroyWindow(_tile_win);
    SDL_DestroyRenderer(_mem_ren);
    SDL_DestroyWindow(_mem_win);
    SDL_DestroyRenderer(_main_ren);
    SDL_DestroyWindow(_main_win);
    SDL_Quit();
  }

  bool is_running() const
  {
    return _running;
  }

  void tick()
  {
    if (_gb.is_v_blank_completed()) {
      SDL_Event event;
      while(SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
          case SDLK_LEFT:  _gb.left(true); break;

          case SDLK_RIGHT: _gb.right(true); break;
          case SDLK_UP:    _gb.up(true); break;
          case SDLK_DOWN:  _gb.down(true); break;

          case SDLK_a:  _gb.a(true); break;
          case SDLK_s:  _gb.b(true); break;
          case SDLK_y:  _gb.select(true); break;
          case SDLK_x:  _gb.start(true); break;
          }
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
          case SDLK_LEFT:  _gb.left(false); break;
          case SDLK_RIGHT: _gb.right(false); break;
          case SDLK_UP:    _gb.up(false); break;
          case SDLK_DOWN:  _gb.down(false); break;

          case SDLK_a:  _gb.a(false); break;
          case SDLK_s:  _gb.b(false); break;
          case SDLK_y:  _gb.select(false); break;
          case SDLK_x:  _gb.start(false); break;
          }
          break;
        case SDL_QUIT:
          _running= false;
          break;
        }
      }

      if (_main_ren) {
        _render_main(_main_ren, _gb);
        SDL_RenderPresent(_main_ren);
      }

      if (_mem_ren) {
        _render_memory(_mem_ren, _gb);
        SDL_RenderPresent(_mem_ren);
      }

      if (_tile_ren) {
        _render_tiles(_tile_ren, _gb);
        SDL_RenderPresent(_tile_ren);
      }
    }
  }

private:
  void _init_sdl()
  {
    if (SDL_Init(SDL_INIT_VIDEO) == -1) {
      std::cerr << "Konnte SDL nicht initialisieren! Fehler: " << SDL_GetError() << std::endl;
      SDL_Quit();
    }
  }

  void _init_main()
  {
    _main_win = SDL_CreateWindow(
      "GB Emulator",
      100,
      100,
      160 * _scale,
      144 * _scale,
      SDL_WINDOW_SHOWN);

    if (_main_win == nullptr) {
      std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
    }

    _main_ren = SDL_CreateRenderer(_main_win, -1, 0);
    if (_main_ren == nullptr) {
      std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
    }

    SDL_RenderClear(_main_ren);
  }

  void _init_mem()
  {
    _mem_win = SDL_CreateWindow(
      "Memory View",
      160*2 + 200,
      100,
      256*3,
      256*3,
      SDL_WINDOW_SHOWN);

    if (_mem_win == nullptr) {
      std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
    }

    _mem_ren = SDL_CreateRenderer(_mem_win, -1, 0);
    if (_mem_ren == nullptr) {
      std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
    }

    SDL_RenderClear(_mem_ren);
  }

  void _init_tile()
  {
    _tile_win = SDL_CreateWindow(
      "Tile View",
      100,
      144*2 + 200,
      8*16,
      8*16,
      SDL_WINDOW_SHOWN);

    if (_tile_win == nullptr) {
      std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
    }

    _tile_ren = SDL_CreateRenderer(_tile_win, -1, 0);
    if (_tile_ren == nullptr) {
      std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
      SDL_Quit();
    }

    SDL_RenderClear(_tile_ren);
  }

  void _render_main(SDL_Renderer* r, GB const& gb)
  {
    SDL_Rect rect;
    rect.w = _scale;
    rect.h = _scale;

    for (int y = 0; y < 144; ++y) {
      for (int x = 0; x < 160; ++x) {
        switch(gb.pixel(x, y)) {
        case 3: // black
          SDL_SetRenderDrawColor(r,   0,   0,   0, 255);
          break;
        case 2: // dark grey
          SDL_SetRenderDrawColor(r, 150, 150, 150, 255);
          break;
        case 1: // light grey
          SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
          break;
        default: // white
          SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
          break;
        }

        rect.x = x * rect.w;
        rect.y = y * rect.h;

        SDL_RenderFillRect(r, &rect);
      }
    }
  }

  void _render_tiles(SDL_Renderer* r, GB const& gb)
  {
    for (int i = 0; i < 256; ++i) {
      auto const x = i%16*8;
      auto const y = i/16*8;
      _render_tile(i, x, y, r, gb);
    }
  }

  void _render_tile(int n, int off_x, int off_y, SDL_Renderer* r, GB const& gb)
  {
    int y = 0;
    for (int i = 0; i < 16; i += 2) {
      auto const byte1 = gb.mem(0x8000 + i + 0 + (n*16));
      auto const byte2 = gb.mem(0x8000 + i + 1 + (n*16));
      int x = 0;
      for (int b = 7; b >= 0; --b) {
        bool const bit1 = byte1 & (1 << b);
        bool const bit2 = byte2 & (1 << b);
        switch((static_cast<unsigned int>(bit2) << 1) | static_cast<unsigned int>(bit1)) {
        case 3: // black
          SDL_SetRenderDrawColor(r,   0,   0,   0, 255);
          break;
        case 2: // dark grey
          SDL_SetRenderDrawColor(r,  85,  85,  85, 255);
          break;
        case 1: // light grey
          SDL_SetRenderDrawColor(r, 171, 171, 171, 255);
          break;
        default: // white
          SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
          break;
        }

        SDL_RenderDrawPoint(r, off_x + x, off_y + y);

        ++x;
      }
      ++y;
    }
  }

  void _render_memory(SDL_Renderer* r, GB const& gb)
  {
    SDL_SetRenderDrawColor(r, 0, 0, 255, 255);
    SDL_RenderClear(r);

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(r, &width, &height);

    SDL_Rect rect;
    rect.w = 3;
    rect.h = 3;
    width /= rect.w;
    height /= rect.h;

    for (GB::wide_reg_t i = 0; i < 0xFFFF; ++i) {
      auto const y = i / width;
      auto const x = i % width;
      auto const val = gb.mem(i);
      if (i >= 0x8000 and i <= 0x8FFF) {
        SDL_SetRenderDrawColor(r, 0, val, 0, 255);
      }
      else if (i >= 0xFE00 and i < 0xFEA0) {
        SDL_SetRenderDrawColor(r, 100, 100, val, 255);
      }
      else {
        SDL_SetRenderDrawColor(r, val, 0, 0, 255);
      }

      rect.x = x * rect.w;
      rect.y = y * rect.h;
      SDL_RenderFillRect(r, &rect);
    }
  }

private:
  GB& _gb;

  bool      _running = true;
  int const _scale;

  SDL_Window*   _main_win = nullptr;
  SDL_Renderer* _main_ren = nullptr;

  SDL_Window*   _mem_win = nullptr;
  SDL_Renderer* _mem_ren = nullptr;

  SDL_Window*   _tile_win = nullptr;
  SDL_Renderer* _tile_ren = nullptr;
};
