#pragma once

#include "types.h"
#include "mm.hpp"
#include "gr.hpp"
#include "cp.hpp"
#include "input.hpp"
#include "timer.hpp"

#include <string>

#include <stdio.h>

class GB
{
public:
  typedef uint8_t            reg_t;
  typedef uint16_t           wide_reg_t;
  typedef std::vector<reg_t> cartridge_t;
  typedef std::vector<reg_t> mem_t;

  Error insert_rom(cartridge_t const& cartridge)
  {
    return _mm.insert_rom(cartridge);
  }

  void power_on()
  {
    _mm.power_on();
    _cp.power_on();
    _t.power_on();
    _in.power_on();
  }

  void left(bool down) { _in.left(down); }
  void right(bool down) { _in.right(down); }
  void up(bool down) { _in.up(down); }
  void down(bool down) { _in.down(down); }

  void a(bool down) { _in.a(down); }
  void b(bool down) { _in.b(down); }
  void start(bool down) { _in.start(down); }
  void select(bool down) { _in.select(down); }

  reg_t mem(wide_reg_t addr) const
  {
    return _mm.read(addr);
  }

  reg_t screen_width() const
  {
    return _gr.width();
  }

  reg_t screen_height() const
  {
    return _gr.height();
  }

  GR::screen_t screen() const
  {
    return _gr.screen();
  }

  bool is_v_blank_completed() const
  {
    return _gr.lx() == 0 and _gr.ly() == 0;
  }

  void tick()
  {
    if (not _mm.is_rom_verified() and _cp.pc() >= 0x0100) {
      _mm.rom_verified();
    }

    _cp.tick();
    _in.tick();
    _t.tick();
    _gr.tick();

    // FIXME: remove this serial dbg hack
    if (_mm.read(0xFF02)) {
      _mm.write(0xFF02, 0x00);
      printf("SERIAL:%c\n", _mm.read(0xFF01));
    }
  }

  void dbg()
  {
    _cp.dbg();
  }

private:
  MM      _mm;
  CP      _cp      = { _mm };
  GR      _gr      = { _mm };
  Timer   _t       = { _mm };
  Input   _in      = { _mm };
};
