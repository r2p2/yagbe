#pragma once

#include "types.h"

#include "mm.hpp"

class GR
{
  static const reg_t WIDTH  = 160;
  static const reg_t HEIGHT = 144;

public:
  typedef std::array<reg_t, WIDTH*HEIGHT> screen_t;

  GR(MM& mm)
    : _mm(mm)
    , _lx(0)
  {}

  void power_on()
  {
    _lx = 0;
    _screen  = screen_t();
  }

  screen_t screen() const
  {
    return _screen;
  }

  reg_t width() const
  {
    return WIDTH;
  }

  reg_t height() const
  {
    return HEIGHT;
  }

  reg_t lcdc()    const { return _mm.read(0xFF40); }
  reg_t scy()     const { return _mm.read(0xFF42); }
  reg_t scx()     const { return _mm.read(0xFF43); }
  reg_t wy()      const { return _mm.read(0xFF4A); }
  reg_t wx()      const { return _mm.read(0xFF4B); }
  reg_t ly()      const { return _mm.read(0xFF44); }
  reg_t lyc()     const { return _mm.read(0xFF45); }
  wide_reg_t lx() const { return _lx; }

  void ly(reg_t val)
  {
    _mm.write(0xFF44, val, true);
  }

  void tick()
  {
    auto v_ly = ly();
    auto const v_ly_orig = v_ly;

    ++_lx;

    if (_lx == 450) {
      ++v_ly;
      _lx = 0;
    }

    if (v_ly == 154) {
      v_ly = 0;
    }

    bool mode_entered = false;

    reg_t mode = 0x00;
    if (v_ly >= 144) {
      mode = 0x01;
    }
    else if (lx() == 360) {
      mode = 0x02;
      _render_scanline(v_ly);
      mode_entered = true;
    }
    else if (lx() > 360) {
      mode = 0x02;
    }
    else if (lx() >= 160) {
      mode = 0x00;
    }
    else if (lx() == 0) {
      mode = 0x03;
      mode_entered = true;
      _render_background(lx(), v_ly);
    }
    else {
      mode = 0x03;
      _render_background(lx(), v_ly);
    }

    reg_t ly_lyc = (v_ly == lyc()) << 2;
    reg_t stat = _mm.read(0xFF41) & 0xF8;
    _mm.write(0xFF41, stat | ly_lyc | mode);

    auto const int_00 = stat & 0x08;
    auto const int_01 = stat & 0x10;
    auto const int_10 = stat & 0x20;
    auto const int_ly = stat & 0x40;

    bool interrupt =
      (mode == 0x00 and int_00) or
      (mode == 0x01 and int_01) or
      (mode == 0x10 and int_10) or
      (ly_lyc       and int_ly);

    if (mode_entered and interrupt) {
      _mm.write(0xFF0F, _mm.read(0xFF0F) | 0x02); // LCDC int
    }

    if (v_ly == 144 and lx() == 0) {
      _mm.write(0xFF0F, _mm.read(0xFF0F) | 0x01); // vblank
    }

    if (v_ly != v_ly_orig)
      ly(v_ly);
  }

private:
  reg_t _pixel_tile(
    reg_t index,
    int x,
    int y,
    bool tds = false,
    bool x_flip = false,
    bool y_flip = false) const
  {
    reg_t tile_byte_1 = 0;
    reg_t tile_byte_2 = 0;

    int const tile_y = y_flip ? (7-y) : y;
    if (tds) {
      tile_byte_1 = _mm.read(0x8000 + (index*16) + tile_y*2 + 0);
      tile_byte_2 = _mm.read(0x8000 + (index*16) + tile_y*2 + 1);
    }
    else {
      tile_byte_1 = _mm.read(0x9000 + (static_cast<int8_t>(index)*16) + y*2 + 0);
      tile_byte_2 = _mm.read(0x9000 + (static_cast<int8_t>(index)*16) + y*2 + 1);
    }

    int   const bit         = x_flip ? x : (7 - x);
    reg_t const pixel_bit_1 = (tile_byte_1 & (1 << bit)) > 0;
    reg_t const pixel_bit_2 = (tile_byte_2 & (1 << bit)) > 0;

    return (pixel_bit_2 << 1) | pixel_bit_1;
  }

  void _render_scanline(int line)
  {
    auto const v_lcdc = lcdc();
    if (not (v_lcdc & 0x80))
      return;

    if (v_lcdc & 0x02)
       _render_sprites(line);

    if (v_lcdc & 0x20)
      _render_window(line);
  }

  void _render_window(int line)
  {
    int const v_wx = wx() - 7;
    int const v_wy = wy();

    if (wx() > 166 or wy() >= 143 or line < v_wy or (line > v_wy + 144))
      return;

    for (int x = 0; x < 160; ++x) {
      if (x < v_wx or x > (v_wx + 166))
        continue;

      _screen[line * width() + x] = _map_palette(0xFF47, _pixel_window(x, line));
    }
  }

  void _render_sprites(int line)
  {
    bool const small_sprites = not (lcdc() & 0x04);
    reg_t sprite_count = 0;

    for (int i = 0; i < 40 and sprite_count < 11; ++i) {
      auto const oam_addr = 0xFE00 + i*4;
      reg_t const s_y = _mm.read(oam_addr + 0);
      reg_t const s_x = _mm.read(oam_addr + 1);
      reg_t const s_n = _mm.read(oam_addr + 2);
      reg_t const c   = _mm.read(oam_addr + 3);

      bool const xf   =      c & 0x20;
      bool const yf   =      c & 0x40;
      bool const pal  =      c & 0x10;
      bool const prio = not (c & 0x80);

      if (s_y == 0 or s_x == 0) {
        continue;
      }

      int const right_x = s_x;
      int const left_x  = s_x - 8;

      int const bottom_y = small_sprites ? s_y - 9 : s_y-1;
      int const top_y = s_y - 16;

      if (line < top_y or line > bottom_y)
        continue;

      ++sprite_count;

      for (int x = 0; x < 8; ++x) {
        auto const screen_x = left_x + x;
        if (screen_x < 0 or screen_x >= 160)
          continue;

	auto const dot_color =
	  _pixel_tile(s_n, x, line - top_y, true, xf, yf);
	if (dot_color == 0)
	  continue;

        auto const new_color =
	  _map_palette(0xFF48 + pal, dot_color);

        auto& pixel = _screen[line * width() + left_x+x];
        if (prio or pixel == 0)
          pixel = new_color;
      }
    }
  }

  void _render_background(int x, int y)
  {
    auto const v_lcdc = lcdc();
    if (not (v_lcdc & 0x01) or not (v_lcdc & 0x80))
      return;

    _screen[y * width() + x] = _map_palette(0xFF47, _pixel_background(x, y));
  }

  reg_t _pixel_background(int x, int y) const
  {
    bool tile_data_select = (lcdc() & 0x10);
    bool tile_map_select  = (lcdc() & 0x08);

    int tile_map_start = tile_map_select == false ? 0x9800 : 0x9C00;

    int const background_x = (x + scx()) % 256;
    int const background_y = (y + scy()) % 256;

    int const tile_x = background_x / 8;
    int const tile_y = background_y / 8;

    int const tile_local_x = background_x % 8;
    int const tile_local_y = background_y % 8;

    int const tile_data_table_index = tile_y * 32 + tile_x;
    int const tile_index = _mm.read(tile_map_start + tile_data_table_index);

    return _pixel_tile(tile_index, tile_local_x, tile_local_y, tile_data_select, false);
  }

  reg_t _pixel_window(int x, int y) const
  {
    bool tile_data_select = (lcdc() & 0x10);
    int tile_map_start = ((lcdc() & 0x40) == 0) ? 0x9800 : 0x9C00;

    int const bg_x = (x - wx() + 6);
    int const bg_y = (y - wy());

    int const tile_x = bg_x / 8;
    int const tile_y = bg_y / 8;

    int const tile_local_x = bg_x % 8;
    int const tile_local_y = bg_y % 8;

    int const tile_data_table_index = tile_y * 32 + tile_x;
    int const tile_index = _mm.read(tile_map_start + tile_data_table_index);

    return _pixel_tile(tile_index, tile_local_x, tile_local_y, tile_data_select);
  }

  reg_t _map_palette(wide_reg_t addr, reg_t value)
  {
    return (_mm.read(addr) >> (2*value)) & 0x03;
  }

private:
  MM&       _mm;

  int       _lx;

  screen_t  _screen;
};
