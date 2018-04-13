#pragma once

class Channel
{
public:
  virtual void power_on() = 0;
  virtual void tick() = 0;
  virtual std::vector<float> wave() const = 0;
  virtual void clear_wave() = 0;

  virtual ~Channel() = default;
};
