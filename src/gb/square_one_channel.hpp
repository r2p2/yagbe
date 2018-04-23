#pragma once

#include "channel.hpp"

class SquareOneChannel : public Channel
{
public:
  explit SquareOneChannel(MM& mm)
    : _mm(mm)
  {
  }

  void power_on() override
  {
    _length_counter_clock.frequency(256);
    _length_counter_clock.reset();
  }

  void tick() override
  {
    _length_counter_clock.tick();

    auto const nr11 = _mm.read(0xFF11);

    auto length_counter = nr11 & 0x0FFF;
    if (length_counter and _length_counter_clock.is_active()) {
      --length_counter;
      _mm.write(0xFF11, (nr11 & 0xF000) | length_counter, true);
    }

    
  }

  std::vector<float> wave() const override
  {
    return _wave;
  }

  void clear_wave() override
  {
    _wave.clear();
  }
private:
  std::array<std::array<bool, 27>, 4> _wave_forms = {{
      {0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
      {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1},
      {0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1},
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1}
    }};

  MM&                _mm;
  Clock              _length_counter_clock; // disable channel if 0

  std::vector<float> _wave;
};
