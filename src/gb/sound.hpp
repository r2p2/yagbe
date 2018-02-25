#pragma once

#include "mm.hpp"
#include "clock.hpp"

#include <vector>

class Sound
{
public:
  Sound(MM& mm)
    : _mm(mm)
  {
  }

  void power_on()
  {
    _length_counter_clock.frequency(256);
    _length_counter_clock.reset();
  }

  void tick()
  {
    _length_counter_clock.tick();

    _gen_wave();
  }

  std::vector<float> wave() const
  {
    return _wave;
  }

  void clear()
  {
    _wave.clear();
  }

private:
  void _gen_wave()
  {
    _wave_clock.tick();
    
    auto length_counter = _mm.read(0xFF1B);

    if (length_counter and _length_counter_clock.is_active()) {
      --length_counter;
      _mm.write(0xFF1B, length_counter, true);
    }

    if (not (_mm.read(0xFF1A) & 0x80)) {
      _wave.push_back(0);
      return;
    }

    int const wave_clock_freq =
      ((_mm.read(0xFF1E) & 0x7) << 8) | _mm.read(0xFF1D);
    _wave_clock.frequency((2048-wave_clock_freq)*2);
    //printf("XXX WCF %d\n", wave_clock_freq);

    if (_wave_clock.is_active()) {
      _wave_pos = (_wave_pos + 1) % 32;
    }

    float vol = 0.0;
    switch ((_mm.read(0xFF1C) & 0x60) >> 5) {
    case 0x00: vol = 0.00; break; //   0% silent
    case 0x01: vol = 1.00; break; // 100%
    case 0x02: vol = 0.50; break; //  50%
    case 0x03: vol = 0.25; break; //  25%
    default: break;
    }

    if (not length_counter) {
      vol = 0.00;
    }

    // 0x00 - 0x0F
    signed int raw_wave_table_val =
      (_mm.read(0xFF30 + _wave_pos/2) >> (_wave_pos % 2 ? 0 : 4)) & 0x0F;

    // -1.0 - +1.0
    float rel_wave_table_val = (2.0 / 16.0 * raw_wave_table_val) - 1;

    // apply volume
    rel_wave_table_val *= vol;

    _wave.push_back(rel_wave_table_val);
    //printf("XXX WTV:%02x %f\n", raw_wave_table_val, rel_wave_table_val);
  }

private:
  MM&                _mm;
  
  std::vector<float> _wave;
  int                _wave_pos;
  Clock              _wave_clock;

  Clock              _length_counter_clock;
};
