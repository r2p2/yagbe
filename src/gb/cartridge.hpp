#pragma once

#include "types.h"
#include "error.hpp"

class Cartridge
{
public:
  enum class MbcType {
    RomOnly,
    Mbc1,
    Mbc2,
    Mbc3,
    Mbc5,
    Mbc6,
    Mbc7,
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
      "CART: mbc type:%d rom banks:%d ram banks:%d\n",
      read(0x0147),
      rom_banks(),
      ram_banks());

    _ram.resize(0x2000 * ram_banks());

    _ram_enabled = false;   // FIXME: separate mbc
    _rom_mode    = true;    // FIXME: separate mbc
    _low_rom_bank_addr = 1; // FIXME: separate mbc 
    _high_bank_addr = 0;    // FIXME: separate mbc
  }

  reg_t read(wide_reg_t addr) const
  {
    if (addr < 0x8000) {
      return read_rom(addr);
    }

    return read_ram(addr); // FIXME: separate mbc
  }

  reg_t read_rom(wide_reg_t addr) const
  {
    auto mapped_addr = 0x0000;

    switch (mbc_type()) {
    case MbcType::Mbc1:
      mapped_addr = _mm_mbc1_rom_mapping(addr);
      break;
    default:
      mapped_addr = addr;
    }

    if (mapped_addr >= _rom.size()) {
      printf("rom oom access %04x => %d\n", addr, mapped_addr);
      return 0x00;
    }

    return _rom[mapped_addr];
  }

  reg_t read_ram(wide_reg_t addr) const
  {
    auto mapped_addr = 0x0000;

    switch (mbc_type()) {
    case MbcType::Mbc1:
      mapped_addr = _mm_mbc1_ram_mapping(addr);
      break;
    default:
      mapped_addr = addr;
    }

    if (mapped_addr >= _ram.size()) {
      printf("ram oom access %04x => %d\n", addr, mapped_addr);
      return 0x00;
    }

    return _ram[mapped_addr];
  }

  void write(wide_reg_t addr, reg_t value)
  {
    if (addr < 0x8000) {
      return write_rom(addr, value);
    }

    return write_ram(addr, value);
  }

  void write_rom(wide_reg_t addr, reg_t value)
  {
    switch (mbc_type()) {
    case MbcType::Mbc1:
      _mm_mbc1(addr, value);
    default:
      return;
    }
  }

  void write_ram(wide_reg_t addr, reg_t value)
  {
    auto mapped_addr = 0x0000;

    switch (mbc_type()) {
    case MbcType::Mbc1:
      mapped_addr = _mm_mbc1_ram_mapping(addr);
      break;
    default:
      mapped_addr = addr;
    }

    if (mapped_addr >= _ram.size()) {
      printf("ram oom access %04x => %d\n", addr, mapped_addr);
      return;
    }

    _ram[mapped_addr] = value;
  }

  MbcType mbc_type() const {
    switch (_rom[0x0147]) {
    case 0x00: return MbcType::RomOnly;
    case 0x01:
    case 0x02:
    case 0x03: return MbcType::Mbc1;
    // case 0x05:
    // case 0x06: return MbcType::Mbc2;
    case 0x08:
    case 0x09: return MbcType::RomRam;
    default:
      return MbcType::Unsupported;
    }
  }

  wide_reg_t rom_banks() const
  {
    switch (_rom[0x0148]) {
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
    switch (_rom[0x0149]) {
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
    return (mbc_type() == MbcType::RomOnly or
       mbc_type() == MbcType::Mbc1);
  }

  bool _mm_mbc1(wide_reg_t addr, reg_t value)
  {
    if (addr >= 0x0000 and addr <= 0x1FFF) {
      _ram_enabled = value == 0xA0;
      return true;
    }

    if (addr >= 0x2000 and addr <= 0x3FFF) {
      _low_rom_bank_addr = value == 0 ? 1 : value;
      return true;
    }

    if (addr >= 0x4000 and addr <= 0x5FFF) {
      _high_bank_addr = value & 0x03;
      return true;
    }

    if (addr >= 0x6000 and addr <= 0x7FFF) {
      _rom_mode = value == 0;
      return true;
    }

    return false;
  }

  size_t _mm_mbc1_rom_mapping(wide_reg_t addr) const
  {
    if (addr >= 0x4000 and addr <= 0x7FFF) {
      return (addr - 0x4000) + 0x4000 * _rom_bank_nr();
    }

    return addr;
  }

  size_t _mm_mbc1_ram_mapping(wide_reg_t addr) const
  {
    if (addr >= 0xA000 and addr <= 0xBFFF) {
      return (addr - 0xA000) + 0x2000 * _ram_bank_nr();
    }

    return addr;
  }

  int _rom_bank_nr() const
  {
    return _low_rom_bank_addr | (_rom_mode ? (_high_bank_addr & 0x03) << 5 : 0);
  }

  int _ram_bank_nr() const
  {
    return _rom_mode ? 0 : _high_bank_addr & 0x03;
  }

private:
  mem_t _rom = mem_t();
  mem_t _ram = mem_t();

  // FIXME special code for mbc1 follows; separate
  bool _ram_enabled       = false;
  bool _rom_mode          = true;
  int  _low_rom_bank_addr = 1;
  int  _high_bank_addr    = 0;
  // FIXME special code ends
};
