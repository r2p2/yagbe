#pragma once

#include "types.h"
#include "mm.hpp"

class Input
{
public:
  Input(MM& mm)
    : _mm(mm)
  {}

  void power_on()
  {
    _left   = false;
    _right  = false;
    _up     = false;
    _down   = false;
    _a      = false;
    _b      = false;
    _start  = false;
    _select = false;
  }

  void tick()
  {
    reg_t p1 =_mm.read(0xFF00) & 0x30;
    bool p14 = not (p1 & 0x10);
    bool p15 = not (p1 & 0x20);

    reg_t buttons = 0x00;
    buttons |= ((p14 and _right) | (p15 and _a))      << 0;
    buttons |= ((p14 and _left)  | (p15 and _b))      << 1;
    buttons |= ((p14 and _up)    | (p15 and _select)) << 2;
    buttons |= ((p14 and _down)  | (p15 and _start))  << 3;

    _mm.write(0xFF00, p1 | (~buttons & 0x0F));

    if (_left or _right or _down or _up or
        _a or _b or _select or _start) {
      _mm.write(0xFF0F, _mm.read(0xFF0F) | 0x10);
    }
  }

  void left(bool down) { _left = down; }
  void right(bool down) { _right = down; }
  void up(bool down) { _up = down; }
  void down(bool down) { _down = down; }

  void a(bool down) { _a = down; }
  void b(bool down) { _b = down; }
  void start(bool down) { _start = down; }
  void select(bool down) { _select = down; }

private:
  MM&  _mm;

  bool _left;
  bool _right;
  bool _up;
  bool _down;
  bool _a;
  bool _b;
  bool _start;
  bool _select;
};
