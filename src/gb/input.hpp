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
    _left           = false;
    _right          = false;
    _up             = false;
    _down           = false;
    _a              = false;
    _b              = false;
    _start          = false;
    _select         = false;
    _button_changed = true;
    _old_p1         = 100;
  }

  void tick()
  {
    reg_t p1 =_mm.read(0xFF00) & 0x30;

    if (_old_p1 == p1 and not _button_changed)
      return;

    _old_p1 = p1;

    bool p14 = not (p1 & 0x10);
    bool p15 = not (p1 & 0x20);

    reg_t buttons = 0x00;
    buttons |= ((p14 and _right) | (p15 and _a))      << 0;
    buttons |= ((p14 and _left)  | (p15 and _b))      << 1;
    buttons |= ((p14 and _up)    | (p15 and _select)) << 2;
    buttons |= ((p14 and _down)  | (p15 and _start))  << 3;

    _mm.write(0xFF00, p1 | (~buttons & 0x0F));

    if (not _button_changed)
      return;

    if (
        (_left   and not _old_left)   or
        (_right  and not _old_right)  or
        (_down   and not _old_down)   or
        (_up     and not _old_up)     or
        (_a      and not _old_a)      or
        (_b      and not _old_b)      or
        (_select and not _old_select) or
        (_start  and not _old_start)) {
      _mm.write(0xFF0F, _mm.read(0xFF0F) | 0x10);
    }

    _old_left       = _left;
    _old_right      = _right;
    _old_up         = _up;
    _old_down       = _down;
    _old_a          = _a;
    _old_b          = _b;
    _old_start      = _start;
    _old_select     = _select;
    _button_changed = false;
  }

  void left(bool down)   { _button_changed = true; _left = down;   }
  void right(bool down)  { _button_changed = true; _right = down;  }
  void up(bool down)     { _button_changed = true; _up = down;     }
  void down(bool down)   { _button_changed = true; _down = down;   }

  void a(bool down)      { _button_changed = true; _a = down;      }
  void b(bool down)      { _button_changed = true; _b = down;      }
  void start(bool down)  { _button_changed = true; _start = down;  }
  void select(bool down) { _button_changed = true; _select = down; }

private:
  MM&   _mm;

  bool  _left;
  bool  _right;
  bool  _up;
  bool  _down;
  bool  _a;
  bool  _b;
  bool  _start;
  bool  _select;

  bool  _old_left;
  bool  _old_right;
  bool  _old_up;
  bool  _old_down;
  bool  _old_a;
  bool  _old_b;
  bool  _old_start;
  bool  _old_select;

  bool  _button_changed;
  reg_t _old_p1;
};
