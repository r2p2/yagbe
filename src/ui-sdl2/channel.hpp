#pragma once

#include <memory>
#include <iostream>

namespace ui
{

class Channel
{
public:
  typedef std::array<float, 735> buffer_t;
  static std::unique_ptr<Channel> build()
  {
    SDL_AudioSpec spec;
    SDL_memset(&spec, 0, sizeof(spec));

    spec.freq     = 44100;
    spec.format   = AUDIO_F32;
    spec.channels = 1;
    spec.samples  = 1024;

    int device_id = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (device_id < 1) {
      std::cerr << SDL_GetError() << std::endl;
      return std::unique_ptr<Channel>();
    }

    return std::make_unique<Channel>(device_id);
  }

  // FIXME do not allow to call this c'tor directly
  explicit Channel(int device_id)
    : _sample_buffer()
    , _device_id(device_id)
  {
    SDL_PauseAudioDevice(_device_id, 0);
  }

  ~Channel()
  {
    SDL_CloseAudioDevice(_device_id);
  }

  void play(std::vector<float> const& samples)
  {
    _down_sample_into_sample_buffer(samples);
    _sync_play_buffer();
    _queue_sample_buffer();
  }

private:
  void _sync_play_buffer()
  {
    while (SDL_GetQueuedAudioSize(_device_id));
  }

  void _down_sample_into_sample_buffer(std::vector<float> const& samples)
  {
    // FIXME improve quality by using different down sampling algorithm
    for (size_t i = 0; i < _sample_buffer.size(); ++i) {
      _sample_buffer[i] = samples[samples.size() * i / _sample_buffer.size()];
    }
  }

  void _queue_sample_buffer()
  {
    auto const vp_buffer = reinterpret_cast<void*>(_sample_buffer.data());
    auto const size      = _sample_buffer.size() * sizeof(buffer_t::value_type);
    if (SDL_QueueAudio(_device_id, vp_buffer, size) != 0) {
      std::cerr << SDL_GetError() << std::endl;
    }
  }

private:
  buffer_t _sample_buffer;
  int      _device_id;
};

} // namespace ui
