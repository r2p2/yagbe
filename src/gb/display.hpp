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
  }

  reg_t width() const
  {
    return WIDTH;
  }

  reg_t height() const
  {
    return HEIGHT;
  }

  void refresh()
  {
    for (size_t i = 0; i < _screen.size(); ++i) {
      auto const x = i % width();
      auto const y = i / width();
      auto const c = _gr.pixel(x, y);

      _screen[i] = c;
    }
  }

  screen_t const& screen() const
  {
    return _screen;
  }

private:
  MM const& _mm;
  GR const& _gr;

  screen_t  _screen;
};
