#pragma once

#include "types.h"

#include "mm.hpp"
#include "gr.hpp"

class Display
{
  static const reg_t WIDTH  = 160;
  static const reg_t HEIGHT = 144;

public:
  typedef std::array<reg_t, WIDTH*HEIGHT> screen_t;

  Display(MM const& mm, GR const& gr)
    : _mm(mm)
    , _gr(gr)
  {}

  void power_on()
  {
    _screen  = screen_t();
    _last_lx = -1;
  }

  reg_t width() const
  {
    return WIDTH;
  }

  reg_t height() const
  {
    return HEIGHT;
  }

  screen_t screen() const
  {
    return _screen;
  }

  void tick()
  {
    auto const x = _gr.lx();

    if (x == _last_lx or x >= width())
      return;

    _last_lx     = x;
    auto const y = _gr.ly();

    if (y >= height())
      return;

    auto const index = y * width() + x;
    auto const color = _gr.pixel(x, y);

    _screen[index] = color;
  }


private:
  MM const& _mm;
  GR const& _gr;

  screen_t  _screen;
  reg_t     _last_lx;
};
