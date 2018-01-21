#pragma once

#include "types.h"
#include "mm.hpp"

#include <array>

class Timer
{
public:
  Timer(MM& mm)
    : _mm(mm)
  {}

  void power_on()
  {
    _cnt = 0;
    _cnt2 = 0;
  }

  void tick()
  {
    ++_cnt2;
    if (_cnt2 > _cls[1]) { // DIV FIXME move into sep. method
      _cnt2 = 0;
      auto div = _mm.read(0xFF04) + 1;
      _mm.write(0xFF04, div, true);
    }

    // FIXME move into sep. method
    auto const tac = _mm.read(0xFF07);
    auto const cls = tac & 0x3;

    if (not (tac & 0x04))
      return;

    ++_cnt;

    if (_cnt > _cls[cls]) {
      _cnt = 0;

      auto tima = _mm.read(0xFF05) + 1;

      if (tima == 0x00) {
        tima = _mm.read(0xFF06);
        _mm.write(0xFF0F, _mm.read(0xFF0F) | 0x04);
      }

      _mm.write(0xFF05, tima);
    }
  }

private:
  MM& _mm;

  int _cnt;
  int _cnt2; // FIXME rename into div_cnt or something

  std::array<int, 4> const _cls = {{ 1024, 16, 64, 256 }};
};
