#include <SDL2/SDL.h>

#include "gb/gb.hpp"
#include "ui-sdl2/ui.hpp"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
  if (argc < 2)
    return 1;

  std::ifstream s_cart(
    argv[1],
    std::ios::in | std::ios::binary);

  GB::cartridge_t cart(
    (std::istreambuf_iterator<char>(s_cart)),
    std::istreambuf_iterator<char>());

  GB gb;
  gb.insert_rom(cart);
  gb.power_on();

  UiSDL ui(gb, 3, false, false);

  int frame = 0;
  auto start = std::chrono::steady_clock::now();
  while(ui.is_running()) {

    do {
      gb.tick();
    }
    while(not gb.is_v_blank_completed());

    frame = (frame + 1) % 4;
    if (frame == 0)
      ui.tick();

    auto const end = std::chrono::steady_clock::now();
    auto const delta = end - start;
    auto const delta_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
    auto const sleep = static_cast<unsigned long>((1000.0/60.0)-delta_ms); // FIXME should be 60
    start = end + std::chrono::milliseconds(sleep);

    if (sleep > 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
  }

  return 0;
}
