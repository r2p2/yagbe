#include <SDL2/SDL.h>

#include "gb/gb.hpp"
#include "ui-sdl2/ui.hpp"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <iterator>

int main(int argc, char** argv)
{
  if (argc < 2)
    return EXIT_FAILURE;

  std::string const rom_path = argv[1];
  std::string const sav_path = rom_path + ".sav"; // FIXME do it properly

  std::ifstream s_cart(
    rom_path,
    std::ios::in | std::ios::binary);

  GB::cartridge_t cart(
    (std::istreambuf_iterator<char>(s_cart)),
    std::istreambuf_iterator<char>());

  GB gb;
  auto const error = gb.insert_rom(cart);
  if (error.is_set()) {
    printf("%s\n", error.text().c_str());
    return EXIT_FAILURE;
  }

  std::ifstream s_sav_in(
    sav_path,
    std::ios::in | std::ios::binary);

  GB::mem_t sav(
    (std::istreambuf_iterator<char>(s_sav_in)),
    std::istreambuf_iterator<char>());
  
  gb.load_ram(sav);
  gb.power_on();

  UiSDL ui(gb, 3, false, false);

  int frame = 0;
  auto start = std::chrono::steady_clock::now();
  while(ui.is_running()) {

    do {
      gb.tick();
    }
    while(not gb.is_v_blank_completed());
    ui.tick();
    //gb.clear_sound();

    auto const end = std::chrono::steady_clock::now();
    auto const delta = end - start;
    auto const delta_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
    auto const sleep = static_cast<unsigned long>((1000.0/60.0)-delta_ms); // FIXME should be 60
    start = end + std::chrono::milliseconds(sleep);

    //    if (sleep > 0)
    // std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
  }

  std::ofstream s_sav_out(
    sav_path,
    std::ios::out | std::ios::binary);
  std::ostream_iterator<char> s_sav_out_it(s_sav_out);

  auto const ram = gb.ram();
  std::copy(ram.begin(), ram.end(), s_sav_out_it);

  return EXIT_SUCCESS;
}
