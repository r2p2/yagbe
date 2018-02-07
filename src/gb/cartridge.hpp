#pragma once

#include "types.h"
#include "error.hpp"

class Cartridge
{
public:
  enum class McbType {
    RomOnly,
    Mcb1,
    Mcb2,
    Mcb3,
    Mcb5,
    Mcb6,
    Mcb7,
    RomRam,
    Mmm,
    Unsupported,
  };

  Error load(std::vector<reg_t> const& data)
  {
    _rom = data;

    if (not _is_rom_supported())
      return Error(Error::Code::RomNotSupported);

    return Error::NoError();
  }

  void power_on()
  {
    printf(
      "CART: mcb type:%d rom banks:%d ram banks:%d\n",
      read(0x0147),
      rom_banks(),
      ram_banks());
  }

  reg_t read(wide_reg_t addr) const
  {
    if (addr >= _rom.size())
      return 0x00;

    return _rom[addr];
  }

  void write(wide_reg_t addr, reg_t value)
  {
    if (addr >= _rom.size())
      return;

    if (addr < 0x8000) // FIXME remove when implementing mcbx
      return;

    _rom[addr] = value;
  }

  McbType mcb_type() const {
    switch (read(0x0147)) {
    case 0x00: return McbType::RomOnly;
    case 0x01:
    case 0x02:
    case 0x03: return McbType::Mcb1;
    // case 0x05:
    // case 0x06: return McbType::Mcb2;
    case 0x08:
    case 0x09: return McbType::RomRam;
    default:
      return McbType::Unsupported;
    }
  }

  wide_reg_t rom_banks() const
  {
    switch (read(0x0148)) {
    case 0x01: return 4;
    case 0x02: return 8;
    case 0x03: return 16;
    case 0x04: return 32;
    case 0x05: return 64;
    case 0x06: return 128;
    case 0x07: return 256;
    case 0x08: return 512;
    case 0x52: return 72;
    case 0x53: return 80;
    case 0x54: return 96;
    default:
      return 0;
    }
  }

  wide_reg_t ram_banks() const
  {
    switch (read(0x0149)) {
    case 0x01: return 1;
    case 0x02: return 1;
    case 0x03: return 4;
    case 0x04: return 16;
    case 0x05: return 8;
    default:
      return 0;
      }
  }

private:
  bool _is_rom_supported() const
  {
    return rom_banks() == 0 and
      ram_banks() == 0 and
      (mcb_type() == McbType::RomOnly or
       mcb_type() == McbType::Mcb1);
  }

private:
  mem_t _rom = mem_t();
};
