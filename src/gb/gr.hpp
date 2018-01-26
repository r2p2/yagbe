#pragma once

#include "types.h"

#include "mm.hpp"

class GR
{
public:
  GR(MM& mm)
    : _mm(mm)
    , _ck(0)
    , _lx(0)
  {}

  void power_on()
  {
    _ck = 0;
    _lx = 0;
  }

  reg_t lcdc() const { return _mm.read(0xFF40); }
  reg_t scy()  const { return _mm.read(0xFF42); }
  reg_t scx()  const { return _mm.read(0xFF43); }
  reg_t wy()   const { return _mm.read(0xFF4A); }
  reg_t wx()   const { return _mm.read(0xFF4B); }
  reg_t ly()   const { return _mm.read(0xFF44); }
  reg_t lyc()  const { return _mm.read(0xFF45); }
  reg_t lx()   const { return _lx; }

  void ly(reg_t val)
  {
    _mm.write(0xFF44, val);
  }

  void tick()
  {
    ++_ck;
    if (_ck == 1) {
      _ck = 0;
      ++_lx;
    }

    if (_lx == (160 + 10)) {
      ly(ly() + 1);
      _lx = 0;
    }

    if (ly() == 144 + 10) {
      ly(0);
    }

    reg_t stat = _mm.read(0xFF41) & 0xF8;
    reg_t mode = 0x00;
    if (ly() >= 144) {
      mode = 0x01;
    }
    else if (lx() >= 160) {
      mode = 0x00;
    }
    else {
      mode = 0x03;
    }

    reg_t ly_lyc = (ly() == lyc()) << 2;

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

    if (_ck == 0 and interrupt) {
      _mm.write(0xFF0F, _mm.read(0xFF0F) | 0x02); // LCDC int
    }

    if (ly() == 144 and lx() == 0 and _ck == 0) {
      _mm.write(0xFF0F, _mm.read(0xFF0F) | 0x01); // vblank
    }
  }

  reg_t pixel(int x, int y) const
  {
    bool const sc_on  = (lcdc() & 0x80);
    bool const sp_on  = (lcdc() & 0x02);
    bool const win_on = (lcdc() & 0x20);
    bool const bg_on  = (lcdc() & 0x01);

    if (not sc_on) {
      return 0x00;
    }

    reg_t bg = bg_on  ? _pixel_background(x, y) : 0;
    reg_t sp = sp_on  ? _pixel_sprite(x, y) : 0;
    reg_t wd = win_on ? _pixel_window(x, y) : 0;

    if (wd > 0)
      return wd;

    if (sp > 0)
      return sp;

    return bg;
  }

private:
  reg_t _pixel_tile(
    wide_reg_t index,
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
      tile_byte_1 = _mm.read(0x9000 + (static_cast<uint16_t>(index)*16) + y*2 + 0);
      tile_byte_2 = _mm.read(0x9000 + (static_cast<uint16_t>(index)*16) + y*2 + 1);
    }

    int   const bit         = x_flip ? x : (7 - x);
    reg_t const pixel_bit_1 = (tile_byte_1 & (1 << bit)) > 0;
    reg_t const pixel_bit_2 = (tile_byte_2 & (1 << bit)) > 0;

    return (pixel_bit_1 << 1) | pixel_bit_2;
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

	if (bg_x < 0 or bg_x >= 256 or bg_y < 0 or bg_y >= 256)
		return 0x00;

    int const tile_x = bg_x / 8;
    int const tile_y = bg_y / 8;

    int const tile_local_x = bg_x % 8;
    int const tile_local_y = bg_y % 8;

    int const tile_data_table_index = tile_y * 32 + tile_x;
    int const tile_index = _mm.read(tile_map_start + tile_data_table_index);

    return _pixel_tile(tile_index, tile_local_x, tile_local_y, tile_data_select);
  }

  reg_t _pixel_sprite(int x, int y) const
  {
    bool const small_sprites = not (lcdc() & 0x04);
    reg_t color = 0x00;

    for (int i = 0; i < 40; ++i) { // FIXME other direction
      reg_t const s_y = _mm.read(0xFE00 + i*4 + 0);
      reg_t const s_x = _mm.read(0xFE00 + i*4 + 1);
      reg_t const s_n = _mm.read(0xFE00 + i*4 + 2);
      reg_t const c   = _mm.read(0xFE00 + i*4 + 3);

      bool const xf   = c & 0x20;
      bool const yf   = c & 0x40;

      if (s_y == 0 or s_x == 0) {
        continue;
      }

      reg_t const o_x = x - (s_x -  8);
      reg_t const o_y = y - (s_y - 16);

      if (o_x >= 8 or o_y >= 16 or (small_sprites and o_y >= 8)) {
        continue;
      }

      // printf("XXX %03d %03d\n", o_x, o_y);

      // FIXME only 10 per scanline
      color = _pixel_tile(s_n, o_x, o_y, true, xf, yf);
    }
    return color;
  }

private:
  MM& _mm;

  int _ck;
  int _lx;
};
