#pragma once

class MBC
{
public:
  virtual ~MBC() = default;

  virtual reg_t read(wide_reg_t addr) const = 0;
  virtual void  write(wide_reg_t addr, reg_t value) = 0;
  virtual std::string name() const = 0;
};

class MBCRomOnly : public MBC
{
public:
  MBCRomOnly(mem_t const& rom)
    : _rom(rom)
  {
  }

  reg_t read(wide_reg_t addr) const override
  {
    return _rom[addr];
  }

  void write(wide_reg_t /*addr*/, reg_t /*value*/) override
  {
  }

  std::string name() const override
  {
    return "Rom";
  }

private:
  mem_t const& _rom;
};

class MBC1 : public MBC
{
  enum class Mode
  {
    Ram,
    Rom,
  };

public:
  MBC1(mem_t const& rom, mem_t& ram)
    : _mode(Mode::Rom)
    , _low(1)
    , _high(0)
    , _rom(rom)
    , _ram(ram)
  {
  }

  reg_t read(wide_reg_t addr) const override
  {
    // FIXME: handle oom access

    if (addr < 0x8000)
      return _rom[_map_rom_addr(addr)];

    return _ram[_map_ram_addr(addr)];
  }

  void write(wide_reg_t addr, reg_t value) override
  {
    if (addr > 0x8000) {
      _ram[_map_ram_addr(addr)] = value;
      return;
    }

    if (addr >= 0x2000 and addr <= 0x3FFF) {
      _low = value == 0 ? 1 : value;
      return;
    }

    if (addr >= 0x4000 and addr <= 0x5FFF) {
      _high = value & 0x03;
      return;
    }

    if (addr >= 0x6000 and addr <= 0x7FFF) {
      _mode = value == 0 ? Mode::Rom : Mode::Ram;
      return;
    }
  }

  std::string name() const override
  {
    return "MBC1";
  }

private:
  int _rom_bank_nr() const
  {
    switch (_mode) {
    case Mode::Ram:
      return _low;
    case Mode::Rom:
      return _low | ((_high & 0x03) << 5);
    }
  }

  int _ram_bank_nr() const
  {
    switch (_mode) {
    case Mode::Ram:
      return _high & 0x03;
    case Mode::Rom:
      return 0x00;
    }
  }

  size_t _map_rom_addr(wide_reg_t addr) const
  {
    if (addr < 0x4000 or addr > 0x7FFF)
      return addr;

    return (addr - 0x4000) + 0x4000 * _rom_bank_nr();
  }

  size_t _map_ram_addr(wide_reg_t addr) const
  {
    return (addr - 0xA000) + 0x2000 * _ram_bank_nr();
  }

private:
  Mode         _mode;
  int          _low;
  int          _high;

  mem_t const& _rom;
  mem_t&       _ram;
};

class MBC2 : public MBC
{
public:
  MBC2(mem_t const& rom, mem_t& ram)
    : _rom_bank_nr(1)
    , _rom(rom)
    , _ram(ram)
  {
  }

  reg_t read(wide_reg_t addr) const override
  {
    // FIXME: handle oom access

    if (addr < 0x8000)
      return _rom[_map_rom_addr(addr)];

    return _ram[_map_ram_addr(addr)];
  }

  void write(wide_reg_t addr, reg_t value) override
  {
    if (addr > 0x8000) {
      _ram[_map_ram_addr(addr)] = value;
      return;
    }

    if (addr >= 0x2000 and addr <= 0x3FFF and addr & 0x0100) {
      _rom_bank_nr = value == 0 ? 1 : value;
      return;
    }
  }

  std::string name() const override
  {
    return "MBC2";
  }

private:
  size_t _map_rom_addr(wide_reg_t addr) const
  {
    if (addr < 0x4000 or addr > 0x7FFF)
      return addr;

    return (addr - 0x4000) + 0x4000 * _rom_bank_nr;
  }

  size_t _map_ram_addr(wide_reg_t addr) const
  {
    return (addr - 0xA000) + 0x2000;
  }

private:
  int          _rom_bank_nr;

  mem_t const& _rom;
  mem_t&       _ram;
};

class MBC5 : public MBC
{
public:
  MBC5(mem_t const& rom, mem_t& ram)
    : _ram_bank_nr(1)
    , _rom_bank_nr(0)
    , _rom(rom)
    , _ram(ram)
  {
  }

  reg_t read(wide_reg_t addr) const override
  {
    // FIXME: handle oom access

    if (addr < 0x8000)
      return _rom[_map_rom_addr(addr)];

    return _ram[_map_ram_addr(addr)];
  }

  void write(wide_reg_t addr, reg_t value) override
  {
    if (addr > 0x8000) {
      _ram[_map_ram_addr(addr)] = value;
      return;
    }

    if (addr >= 0x2000 and addr <= 0x3FFF) {
      _rom_bank_nr = value;
      return;
    }

    if (addr >= 0x4000 and addr <= 0x5FFF) {
      _ram_bank_nr = value & 0x0F;
      return;
    }
  }

  std::string name() const override
  {
    return "MBC5";
  }

private:
  size_t _map_rom_addr(wide_reg_t addr) const
  {
    if (addr < 0x4000 or addr > 0x7FFF)
      return addr;

    return (addr - 0x4000) + 0x4000 * _rom_bank_nr;
  }

  size_t _map_ram_addr(wide_reg_t addr) const
  {
    return (addr - 0xA000) + 0x2000 * _ram_bank_nr;
  }

private:
  int          _rom_bank_nr;
  int          _ram_bank_nr;

  mem_t const& _rom;
  mem_t&       _ram;
};
