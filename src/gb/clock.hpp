#pragma once

#include <algorithm> // std::max

class Clock
{
public:
  typedef int Hz;

  void tick()
  {
    int const period = 4194304 / _freq; // FIXME: same with double speed mode?
    _cnt = (_cnt + 1) % period;
  }

  bool is_active()
  {
    return not _cnt;
  }

  void frequency(Hz freq)
  {
    _freq = std::max(1, freq);
  }

  void reset()
  {
    _cnt = 0;
  }

private:
  int  _cnt    = 0;
  Hz   _freq   = 1;

  bool _active = false;
};
