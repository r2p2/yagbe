#pragma once

#include "mm.hpp"
#include "clock.hpp"
#include "wave_channel.hpp"

#include <memory>
#include <vector>

class Sound
{
public:
  Sound(MM& mm)
    : _wave(std::make_unique<WaveChannel>(mm))
  {
  }

  void power_on()
  {
    _wave->power_on();
  }

  void tick()
  {
    _wave->tick();
  }

  std::vector<float> wave_channel() const
  {
    return _wave->wave();
  }

  void clear()
  {
    _wave->clear_wave();
  }

private:
  std::unique_ptr<Channel> _wave;
};
