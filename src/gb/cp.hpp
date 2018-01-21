#pragma once

#include "types.h"
#include "mm.hpp"

#include <map>
#include <map>
#include <string>
#include <functional>

class CP
{
  struct Op
  {
    uint8_t               const code;
    std::string           const desc;
    std::function<void()> const fn;
  };

public:
  CP(MM& mm)
    : _mm(mm)
  {}

  void power_on()
  {
    _cycle  = 0;
    _cycles = 0;
    _halted = false;
    _ime    = false;
    // _flag   = 0;
    _sp     = 0xFFFF;
    _pc     = 0x0100;

    _mm.write(0xFF0F, 0x00); // interrupt flag
    _mm.write(0xFFFF, 0xFF); // interrupt enable

    // check op code configuration
    for (int i = 0; i < _ops.size(); ++i) {
      if (_ops[i].code != i) {
        printf("ERROR: opcode does not match: %02x\n", i);
        exit(5);
      }
    }
  }

  wide_reg_t pc() const { return _pc; }
  wide_reg_t sp() const { return _sp; }
  void sp(wide_reg_t value) { _sp = value; }

  bool zero_flag() const { return f() & (1 << 7); }
  void zero_flag(bool b) { _set_bit(7, b); }

  bool substract_flag() const { return f() & (1 << 6); }
  void substract_flag(bool b) { _set_bit(6, b); }

  bool half_carry_flag() const { return f() & (1 << 5); }
  void half_carry_flag(bool b) { _set_bit(5, b); }

  bool carry_flag() const { return f() & (1 << 4); }
  void carry_flag(bool b) { _set_bit(4, b); }

  reg_t& a() { return _a; }
  reg_t& b() { return _b; }
  reg_t& c() { return _c; }
  reg_t& d() { return _d; }
  reg_t& e() { return _e; }
  reg_t& f() { return _f; }
  reg_t& h() { return _h; }
  reg_t& l() { return _l; }

  reg_t const& a() const { return _a; }
  reg_t const& b() const { return _b; }
  reg_t const& c() const { return _c; }
  reg_t const& d() const { return _d; }
  reg_t const& e() const { return _e; }
  reg_t const& f() const { return _f; }
  reg_t const& h() const { return _h; }
  reg_t const& l() const { return _l; }

  wide_reg_t af() const { return _wide(a(), f()); }
  wide_reg_t bc() const { return _wide(b(), c()); }
  wide_reg_t de() const { return _wide(d(), e()); }
  wide_reg_t hl() const { return _wide(h(), l()); }

  void af(wide_reg_t value) { return _wide(a(), f(), value & 0xFFF0); }
  void bc(wide_reg_t value) { return _wide(b(), c(), value); }
  void de(wide_reg_t value) { return _wide(d(), e(), value); }
  void hl(wide_reg_t value) { return _wide(h(), l(), value); }

  reg_t op() const { return _mm.read(_pc); }
  reg_t b1() const { return _mm.read(_pc + 1); }
  reg_t b2() const { return _mm.read(_pc + 2); }
  wide_reg_t nn() const { return (b2() << 8) | b1(); }

  bool tick()
  {
    if (_cycles > 0) {
      --_cycles;
      return false;
    }

    _process_interrupt();

    if (_halted)
      return false;
    _process_opcode();

    return true;
  }

  void dbg()
  {
    printf(
      "pc:%04x sp:%04x op:%02x,%02x,%02x af:%02x%02x bc:%02x%02x de:%02x%02x hl:%02x%02x %c%c%c%c LCDC:%02x %s\n",
      _pc, _sp, _mm.read(_pc), _mm.read(_pc+1), _mm.read(_pc+2), _a, _f, _b, _c, _d, _e, _h, _l,
      (zero_flag() ? 'Z' : '_'),
      (substract_flag() ? 'S' : '_'),
      (half_carry_flag() ? 'H' : '_'),
      (carry_flag() ? 'C' : '_'),
      _mm.read(0xFF40),
      _ops[ _mm.read(_pc)].desc.c_str());
  }

private:
  void _process_interrupt()
  {
    if (not _ime)
      return;

    auto fn_is_enabled = [this] (reg_t val) -> bool {
      return (_mm.read(0xFFFF) & val) and (_mm.read(0xFF0F) & val);
    };

    if (fn_is_enabled(0x01)) { // vblank
       // printf("INT: VBLANK\n");
       _process_interrupt(0x0040);
       _mm.write(0xFF0F, _mm.read(0xFF0F) ^ 0x01);
       return;
    }

    if (fn_is_enabled(0x02)) { // lcdc
      // printf("INT: LCDC\n");
      _process_interrupt(0x0048);
      _mm.write(0xFF0F, _mm.read(0xFF0F) ^ 0x02);

      return;
    }

    if (fn_is_enabled(0x04)) { // timer
      // printf("INT: TIMER\n");
      _process_interrupt(0x0050);
      _mm.write(0xFF0F, _mm.read(0xFF0F) ^ 0x04);
      return;
    }

    if (fn_is_enabled(0x08)) { // S IO
      // printf("INT: SIO\n");
      _process_interrupt(0x0058);
      _mm.write(0xFF0F, _mm.read(0xFF0F) ^ 0x08);
      return;
    }

    if (fn_is_enabled(0x10)) { // high->low P10-P13
      // printf("INT: P10-13\n");
      _process_interrupt(0x0060);
      _mm.write(0xFF0F, _mm.read(0xFF0F) ^ 0x10);
      return;
    }
  }

  void _process_interrupt(wide_reg_t addr) {
    _ime = false;
    _push(_pc);
    _pc = 0x0040;
    _halted = false; // FIXME where to put this?
  }

  void _process_opcode()
  {
    auto const op_code = op();

    if (op_code == 0xCB) {
      auto fn = _pref.find(b1());
      if (fn != _pref.end()) {
        fn->second();
        return;
      }
      exit(1);
    }

    _ops[op_code].fn();
  }

  wide_reg_t _wide(reg_t const& high, reg_t const& low) const
  {
    return (high << 8) | low;
  }

  void _wide(reg_t& high, reg_t& low, wide_reg_t value)
  {
    high = value >> 8;
    low  = value;
  }

  void _set_bit(int n, bool val) // FIXME: rename
  {
    f() ^= (-static_cast<unsigned long>(val) ^ f()) & (1UL << n);
  }

  inline void _push(wide_reg_t val) // FIXME: dirty
  {
    _push_stack(val >> 8);
    _push_stack(val);
  }

  inline wide_reg_t _pop() // FIXME: dirty
  {
    auto l = _pop_stack();
    auto h = _pop_stack();
    return h << 8 | l;
  }

  void _push_stack(reg_t value) // FIXME: dirty
  {
    --_sp;
    _mm.write(_sp, value);
  }

  reg_t _pop_stack() // FIXME: dirty
  {
    auto const v = _mm.read(_sp);
    ++_sp;
    return v;
  }

  inline void _ld8(reg_t const& src, reg_t& dst)
  {
    dst = src;
  }

  inline void _ld_hl_spn(uint8_t n)
  {
    auto const sp_old = sp();
    sp(static_cast<int8_t>(sp_old) + static_cast<int8_t>(n)); // FIXME wtf

    zero_flag(false);
    substract_flag(false);

    carry_flag(false); // FIXME ???
    half_carry_flag(false); // FIXME ???
  }

  inline void _add8(reg_t n, reg_t& dst)
  {
    half_carry_flag(((dst & 0xF) + (n & 0xF)) > 0x0F);
    carry_flag((static_cast<uint16_t>(dst) + static_cast<uint16_t>(n)) > 0xFF);

    dst += n;

    zero_flag(dst == 0);
    substract_flag(false);
  }

  inline void _inc(reg_t& dst)
  {
    half_carry_flag(((dst & 0xF) + (1 & 0xF)) > 0x0F);

    dst += 1;

    zero_flag(dst == 0);
    substract_flag(false);
  }


  inline wide_reg_t _add16(wide_reg_t n, wide_reg_t dst)
  {
    half_carry_flag(((dst & 0x0FFF) + (n & 0x0FFF)) > 0x0FFF);
    carry_flag((static_cast<uint32_t>(dst) + static_cast<uint32_t>(n)) > 0xFFFF);

    dst += n;

    substract_flag(false);

    return dst;
  }

  inline void _adc8(uint32_t n, reg_t& dst)
  {
    // _add8(carry_flag() ? n + 1 : n, dst); // orig

    // n += static_cast<int>(carry_flag());

    // half_carry_flag(((dst & 0xF) + (n & 0xF)) & 0x10);
    // carry_flag((static_cast<uint16_t>(dst) + static_cast<uint16_t>(n)) & 0x10000);

    // dst += n;

    // zero_flag(dst == 0);
    // substract_flag(false);

    uint32_t result = dst + n;

    if (carry_flag())
      result += 1;

    half_carry_flag(((dst & 0xF) + (n & 0xF) + carry_flag()) > 0x0F);
    carry_flag(result > 0xFF);

    dst = result;

    zero_flag(dst == 0);
    substract_flag(false);
  }

  inline void _sub8(reg_t n, reg_t& dst)
  {
    // half_carry_flag(((dst & 0xF) - (n & 0xF)) < 0);
    // carry_flag((static_cast<uint16_t>(dst) - static_cast<uint16_t>(n)) < 0);

    // dst -= n;

    // zero_flag(dst == 0);
    // substract_flag(true);
    int result = dst - n;

    half_carry_flag(((dst & 0xF) - (n & 0xF)) < 0);
    carry_flag(result < 0);

    dst = result;

    zero_flag(dst == 0);
    substract_flag(true);
  }

  inline void _dec(reg_t& dst)
  {
    int result = dst - 1;

    half_carry_flag(((dst & 0xF) - (1 & 0xF)) < 0);

    dst = result;

    zero_flag(dst == 0);
    substract_flag(true);
  }

  inline void _sbc8(reg_t n, reg_t& dst)
  {
    int result = dst - n - carry_flag();

    half_carry_flag(((dst & 0xF) - (n & 0xF) - carry_flag()) < 0);
    carry_flag(result < 0);

    dst = result;

    zero_flag(dst == 0);
    substract_flag(true);
  }

  inline void _and(reg_t n, reg_t& dst)
  {
    dst = dst & n;

    zero_flag(dst == 0);
    substract_flag(false);
    half_carry_flag(true);
    carry_flag(false);
  }

  inline void _or(reg_t n, reg_t& dst)
  {
    dst = dst | n;

    zero_flag(dst == 0);
    substract_flag(false);
    half_carry_flag(false);
    carry_flag(false);
  }

  inline void _xor(reg_t n, reg_t& dst)
  {
    dst = dst ^ n;

    zero_flag(dst == 0);
    substract_flag(false);
    half_carry_flag(false);
    carry_flag(false);
  }

  inline void _cp(reg_t n)
  {
    zero_flag(a() == n);
    substract_flag(true);
    half_carry_flag(((a() & 0xF) - (n & 0xF)) < 0);
    carry_flag(a() < n);
  }

  inline void _rlc(reg_t& dst, bool zero = false)
  {
    auto const carry = dst & 0x80;
    dst <<= 1;
    dst |= carry >> 7;
    carry_flag(carry);

    if (zero)
      zero_flag(dst == 0);
    else
      zero_flag(false);

    substract_flag(false);
    half_carry_flag(false);
  }

  inline void _rrc(reg_t& dst, bool zero = false)
  {
    auto const carry =  dst & 0x01;
    dst >>= 1;
    dst |= carry << 7;
    carry_flag(carry);

    if (zero)
      zero_flag(dst == 0);
    else
      zero_flag(false);

    substract_flag(false);
    half_carry_flag(false);
  }

  inline void _rl(reg_t& dst, bool zero = false)
  {
    reg_t const old_carry = carry_flag();
    carry_flag(dst & 0x80);
    dst <<= 1;
    dst |= old_carry;

    if (zero)
      zero_flag(dst == 0);
    else
      zero_flag(false);

    substract_flag(false);
    half_carry_flag(false);
  }

  inline void _rr(reg_t& dst, bool zero = false)
  {
    reg_t const old_carry = carry_flag();
    carry_flag(dst & 0x01);
    dst >>= 1;
    dst |= old_carry << 7;

    if (zero)
      zero_flag(dst == 0);
    else
      zero_flag(false);

    substract_flag(false);
    half_carry_flag(false);
  }

  inline void _sla(reg_t& dst)
  {
    carry_flag(dst & 0x80);
    dst <<= 1;

    zero_flag(dst == 0);
    substract_flag(false);
    half_carry_flag(false);
  }

  inline void _sra(reg_t& dst)
  {
    reg_t const old_msb = dst & 0x80;
    carry_flag(dst & 0x01);

    dst >>= 1;
    dst |= old_msb;

    zero_flag(dst == 0);
    substract_flag(false);
    half_carry_flag(false);
  }

  inline void _srl(reg_t& dst)
  {
    carry_flag(dst & 0x01);

    dst >>= 1;

    zero_flag(dst == 0);
    substract_flag(false);
    half_carry_flag(false);
  }

  inline void _bit(reg_t dst, reg_t bit)
  {
    zero_flag((dst & (1 << bit)) == 0);
    substract_flag(false);
    half_carry_flag(true);
  }

  inline void _set(reg_t& dst, reg_t bit)
  {
    dst |= (1 << bit);
  }

  inline void _res(reg_t& dst, reg_t bit)
  {
    dst &= ~(1 << bit);
  }

  inline void _call(wide_reg_t nn, reg_t offset = 3)
  {
    auto const next_pc = _pc + offset;
    _push(next_pc);
    _pc = nn;
  }

  inline void _swap(reg_t& dst)
  {
    dst = ((dst & 0xF0) >> 4) | ((dst & 0x0F) << 4);
    zero_flag(dst == 0);
    carry_flag(false);
    substract_flag(false);
    half_carry_flag(false);
  }

private:
  MM&        _mm;

  reg_t      _a;
  reg_t      _b;
  reg_t      _c;
  reg_t      _d;
  reg_t      _e;
  reg_t      _f;
  reg_t      _g;
  reg_t      _h;
  reg_t      _l;
  // reg_t      _flag;
  wide_reg_t _sp;
  wide_reg_t _pc;

  bool       _ime;
  bool       _halted;

  uint8_t    _cycles; // FIXME: rename to busy_cycles
  uint64_t   _cycle;

  std::array<Op, 0x100> _ops = {{
    {
      0x00,
      "NOP",
      [this]() { _pc += 1; _cycles = 4; }
    },
    {
      0x01,
      "LD BC,nn",
      [this]() { bc(nn()); _pc += 3; _cycles = 12; }
    },
    {
      0x02,
      "LD (BC),A",
      [this]() { reg_t i; _ld8(a(), i); _mm.write(bc(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x03,
      "INC BC",
      [this]() { bc(bc() + 1); _pc += 1; _cycles = 8; }
    },
    {
      0x04,
      "INC B",
      [this]() { _inc(b()); _pc += 1; _cycles = 4; }
    },
    {
      0x05,
      "DEC B",
      [this]() { _dec(b()); _pc += 1; _cycles = 4; }
    },
    {
      0x06,
      "LD B,n",
      [this]() { _ld8(b1(), b()); _pc += 2; _cycles = 8; }
    },
    {
      0x07,
      "RLCA",
      [this]() { _rlc(a()); _pc += 1; _cycles = 4; }
    },
    {
      0x08,
      "LD (nn),SP",
      [this]()
      {
        _mm.write(nn()    ,  sp() & 0x00FF      );
        _mm.write(nn() + 1, (sp() & 0xFF00) >> 8);
        _pc += 3;
        _cycles = 20;
      }
    },
    {
      0x09,
      "ADD HL,BC",
      [this]() { hl(_add16(bc(), hl())); _pc += 1; _cycles = 8; }
    },
    {
      0x0A, "LD A,(BC)",
      [this]() { _ld8(_mm.read(bc()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0x0B,
      "DEC BC",
      [this]() { bc(bc() - 1); _pc += 1; _cycles = 8; }
    },
    {
      0x0C,
      "INC C",
      [this]() { _inc(c()); _pc += 1; _cycles = 4; }
    },
    {
      0x0D,
      "DEC C",
      [this]() { _dec(c()); _pc += 1; _cycles = 4; }
    },
    {
      0x0E,
      "LD C,n",
      [this]() { _ld8(b1(), c()); _pc += 2; _cycles = 8; }
    },
    {
      0x0F,
      "RRCA",
      [this]() { _rrc(a()); _pc += 1; _cycles = 4; }
    },

    { 0x10,
      "STOP",
      [this]() { _halted = true; _pc += 2; _cycles = 4; }
    },
    {
      0x11,
      "LD DE,nn",
      [this]() { de(nn()); _pc += 3; _cycles = 12; }
    },
    {
      0x12,
      "LD (DE),A",
      [this]() { reg_t i; _ld8(a(), i); _mm.write(de(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x13,
      "INC DE",
      [this]() { de(de() + 1); _pc += 1; _cycles = 8; }
    },
    {
      0x14,
      "INC D",
      [this]() { _inc(d()); _pc += 1; _cycles = 4; }
    },
    {
      0x15,
      "DEC D",
      [this]() { _dec(d()); _pc += 1; _cycles = 4; }
    },
    {
      0x16,
      "LD D,n",
      [this]() { _ld8(b1(), d()); _pc += 2; _cycles = 8; }
    },
    {
      0x17,
      "RLA",
      [this]() { _rl(a()); _pc += 1; _cycles = 4; }
    },
    {
      0x18,
      "JR n",
      [this]() { _pc += static_cast<int8_t>(b1()) + 2; _cycles = 8; }
    },
    {
      0x19,
      "ADD HL,DE",
      [this]() { hl(_add16(de(), hl())); _pc += 1; _cycles = 8; }
    },
    {
      0x1A,
      "LD A,(DE)",
      [this]() { _ld8(_mm.read(de()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0x1B,
      "DEC DE",
      [this]() { de(de() - 1); _pc += 1; _cycles = 8; }
    },
    {
      0x1C,
      "INC E",
      [this]() { _inc(e()); _pc += 1; _cycles = 4; }
    },
    {
      0x1D,
      "DEC E",
      [this]() { _dec(e()); _pc += 1; _cycles = 4; }
    },
    {
      0x1E,
      "LD E,n",
      [this]() { _ld8(b1(), e()); _pc += 2; _cycles = 8; }
    },
    {
      0x1F,
      "RRA",
      [this]() { _rr(a()); _pc += 1; _cycles = 4; }
    },

    { 0x20,
      "JR NZ,n",
      [this]()
	  {
		int8_t r = b1();
		if (zero_flag())
          r = 0;
		_pc += r + 2;
		_cycles = 8;
	  }
    },
    {
      0x21,
      "LD HL,nn",
      [this]() { hl(nn()); _pc += 3; _cycles = 12; }
    },
    {
      0x22,
      "LD (HL+),A",
      [this]() { reg_t i = 0; _ld8(a(), i); _mm.write(hl(), i); hl(hl()+1); _pc += 1; _cycles = 8; }
    },
    {
      0x23,
      "INC HL",
      [this]() { hl(hl() + 1); _pc += 1; _cycles = 8; }
    },
    {
      0x24,
      "INC H",
      [this]() { _inc(h()); _pc += 1; _cycles = 4; }
    },
    {
      0x25,
      "DEC H",
      [this]() { _dec(h()); _pc += 1; _cycles = 4; }
    },
    {
      0x26,
      "LD H,n",
      [this]() { _ld8(b1(), h()); _pc += 2; _cycles = 8; }
    },
    {
      0x27,
      "DAA",
      // [this]() { a() = ((a()/10%10)<<4)|((a()%10)&0xF); _pc += 1; _cycles = 4; }
      [this]()
      {
        int va = a();
        if (not substract_flag()) {
          if (half_carry_flag() or (va & 0x0F) > 0x09)
            va += 0x06;

          if (carry_flag() or va > 0x9F) {
            va += 0x60;
          }
        }
        else {
          if (half_carry_flag())
            va = (va - 0x06) & 0xFF;

          if (carry_flag())
            va -= 0x60;
        }

        if ((va & 0x100) == 0x100) {
          carry_flag(true);
        }

        half_carry_flag(false);
        zero_flag((va & 0xFF) == 0);
        a() = va & 0xFF;

        _pc += 1; _cycles = 4;
      }
    },
    {
      0x28,
      "JR Z,n",
      [this]() { auto const d = (zero_flag())      ? static_cast<int8_t>(b1()) : 0; _pc += d + 2; _cycles = 8; }
    },
    {
      0x29,
      "ADD HL,HL",
      [this]() { hl(_add16(hl(), hl())); _pc += 1; _cycles = 8; }
    },
    {
      0x2A,
      "LD A,(HL+)",
      [this]() { _ld8(_mm.read(hl()), a()); hl(hl()+1); _pc += 1; _cycles = 8; }
    },
    {
      0x2B,
      "DEC HL",
      [this]() { hl(hl() - 1); _pc += 1; _cycles = 8; }
    },
    {
      0x2C,
      "INC L",
      [this]() { _inc(l()); _pc += 1; _cycles = 4; }
    },
    {
      0x2D,
      "DEC L",
      [this]() { _dec(l()); _pc += 1; _cycles = 4; }
    },
    {
      0x2E,
      "LD L,n",
      [this]() { _ld8(b1(), l()); _pc += 2; _cycles = 8; }
    },
    {
      0x2F,
      "CPL",
      [this]() { a() = ~a(); substract_flag(true); half_carry_flag(true); _pc += 1; _cycles = 4; }
    },

    { 0x30,
      "JR NC,n",
      [this]() { auto const d = (not carry_flag()) ? static_cast<int8_t>(b1()) : 0; _pc += d + 2; _cycles = 8; }
    },
    {
      0x31,
      "LD SP,nn",
      [this]() { sp(nn()); _pc += 3; _cycles = 12; }
    },
    {
      0x32,
      "LD (HL-),A",
      [this]() { reg_t i = 0; _ld8(a(), i); _mm.write(hl(), i); hl(hl()-1); _pc += 1; _cycles = 8; }
    },
    {
      0x33,
      "INC SP",
      [this]() { sp(sp() + 1); _pc += 1; _cycles = 8; }
    },
    {
      0x34,
      "INC (HL)",
      [this]() { reg_t i = _mm.read(hl()); _inc(i); _mm.write(hl(), i); _pc += 1; _cycles = 12; }
    },
    {
      0x35,
      "DEC (HL)",
      [this]() { reg_t i = _mm.read(hl()); _dec(i); _mm.write(hl(), i); _pc += 1; _cycles = 12; }
    },
    {
      0x36,
      "LD (HL),n",
      [this]() { reg_t i = 0; _ld8(b1(), i); _mm.write(hl(), i); _pc += 2; _cycles = 12; }
    },
    {
      0x37,
      "SCF",
      [this]() { carry_flag(true); substract_flag(false); half_carry_flag(false); _pc += 1; _cycles = 4; }
    },
    {
      0x38,
      "JR C,n",
      [this]() { auto const d = (carry_flag())     ? static_cast<int8_t>(b1()) : 0; _pc += d + 2; _cycles = 8; }
    },
    {
      0x39,
      "ADD HL,SP",
      [this]() { hl(_add16(sp(), hl())); _pc += 1; _cycles = 8; }
    },
    {
      0x3A,
      "LD A,(HL-)",
      [this]() { _ld8(_mm.read(hl()), a()); hl(hl()-1); _pc += 1; _cycles = 8; }
    },
    {
      0x3B,
      "DEC SP",
      [this]() { sp(sp() - 1); _pc += 1; _cycles = 8; }
    },
    {
      0x3C,
      "INC A",
      [this]() { _inc(a()); _pc += 1; _cycles = 4; }
    },
    {
      0x3D,
      "DEC A",
      [this]() { _dec(a()); _pc += 1; _cycles = 4; }
    },
    {
      0x3E,
      "LD A,n",
      [this]() { _ld8(b1(), a()); _pc += 2; _cycles = 8; }
    },
    {
      0x3F,
      "CCF",
      [this]() { substract_flag(false); half_carry_flag(false); carry_flag(carry_flag()?false:true); _pc += 1; _cycles = 4; }
    },

    { 0x40,
      "LD B,B",
      [this]() { _ld8(b(), b()); _pc += 1; _cycles = 4; }
    },
    {
      0x41,
      "LD B,C",
      [this]() { _ld8(c(), b()); _pc += 1; _cycles = 4; }
    },
    {
      0x42,
      "LD B,D",
      [this]() { _ld8(d(), b()); _pc += 1; _cycles = 4; }
    },
    {
      0x43,
      "LD B,E",
      [this]() { _ld8(e(), b()); _pc += 1; _cycles = 4; }
    },
    {
      0x44,
      "LD B,H",
      [this]() { _ld8(h(), b()); _pc += 1; _cycles = 4; }
    },
    {
      0x45,
      "LD B,L",
      [this]() { _ld8(l(), b()); _pc += 1; _cycles = 4; }
    },
    {
      0x46,
      "LD B,(HL)",
      [this]() { _ld8(_mm.read(hl()), b()); _pc += 1; _cycles = 8; }
    },
    {
      0x47,
      "LD B,A",
      [this]() { _ld8(a(), b()); _pc += 1; _cycles = 4; }
    },
    {
      0x48,
      "LD C,B",
      [this]() { _ld8(b(), c()); _pc += 1; _cycles = 4; }
    },
    {
      0x49,
      "LD C,C",
      [this]() { _ld8(c(), c()); _pc += 1; _cycles = 4; }
    },
    {
      0x4A,
      "LD C,D",
      [this]() { _ld8(d(), c()); _pc += 1; _cycles = 4; }
    },
    {
      0x4B,
      "LD C,E",
      [this]() { _ld8(e(), c()); _pc += 1; _cycles = 4; }
    },
    {
      0x4C,
      "LD C,H",
      [this]() { _ld8(h(), c()); _pc += 1; _cycles = 4; }
    },
    {
      0x4D,
      "LD C,L",
      [this]() { _ld8(l(), c()); _pc += 1; _cycles = 4; }
    },
    {
      0x4E,
      "LD C,(HL)",
      [this]() { _ld8(_mm.read(hl()), c()); _pc += 1; _cycles = 8; }
    },
    {
      0x4F,
      "LD C,A",
      [this]() { _ld8(a(), c()); _pc += 1; _cycles = 4; }
    },
    { 0x50,
      "LD D,B",
      [this]() { _ld8(b(), d()); _pc += 1; _cycles = 4; }
    },
    {
      0x51,
      "LD D,C",
      [this]() { _ld8(c(), d()); _pc += 1; _cycles = 4; }
    },
    {
      0x52,
      "LD D,D",
      [this]() { _ld8(d(), d()); _pc += 1; _cycles = 4; }
    },
    {
      0x53,
      "LD D,E",
      [this]() { _ld8(e(), d()); _pc += 1; _cycles = 4; }
    },
    {
      0x54,
      "LD D,H",
      [this]() { _ld8(h(), d()); _pc += 1; _cycles = 4; }
    },
    {
      0x55,
      "LD D,L",
      [this]() { _ld8(l(), d()); _pc += 1; _cycles = 4; }
    },
    {
      0x56,
      "LD D,(HL)",
      [this]() { _ld8(_mm.read(hl()), d()); _pc += 1; _cycles = 8; }
    },
    {
      0x57,
      "LD D,A",
      [this]() { _ld8(a(), d()); _pc += 1; _cycles = 4; }
    },
    {
      0x58,
      "LD E,B",
      [this]() { _ld8(b(), e()); _pc += 1; _cycles = 4; }
    },
    {
      0x59,
      "LD E,C",
      [this]() { _ld8(c(), e()); _pc += 1; _cycles = 4; }
    },
    {
      0x5A,
      "LD E,D",
      [this]() { _ld8(d(), e()); _pc += 1; _cycles = 4; }
    },
    {
      0x5B,
      "LD E,E",
      [this]() { _ld8(e(), e()); _pc += 1; _cycles = 4; }
    },
    {
      0x5C,
      "LD E,H",
      [this]() { _ld8(h(), e()); _pc += 1; _cycles = 4; }
    },
    {
      0x5D,
      "LD E,L",
      [this]() { _ld8(l(), e()); _pc += 1; _cycles = 4; }
    },
    {
      0x5E,
      "LD E,(HL)",
      [this]() { _ld8(_mm.read(hl()), e()); _pc += 1; _cycles = 8; }
    },
    {
      0x5F,
      "LD E,A",
      [this]() { _ld8(a(), e()); _pc += 1; _cycles = 4; }
    },

    { 0x60,
      "LD H,B",
      [this]() { _ld8(b(), h()); _pc += 1; _cycles = 4; }
    },
    {
      0x61,
      "LD H,C",
      [this]() { _ld8(c(), h()); _pc += 1; _cycles = 4; }
    },
    {
      0x62,
      "LD H,D",
      [this]() { _ld8(d(), h()); _pc += 1; _cycles = 4; }
    },
    {
      0x63,
      "LD H,E",
      [this]() { _ld8(e(), h()); _pc += 1; _cycles = 4; }
    },
    {
      0x64,
      "LD H,H",
      [this]() { _ld8(h(), h()); _pc += 1; _cycles = 4; }
    },
    {
      0x65,
      "LD H,l",
      [this]() { _ld8(l(), h()); _pc += 1; _cycles = 4; }
    },
    {
      0x66,
      "LD H,(HL)",
      [this]() { _ld8(_mm.read(hl()), h()); _pc += 1; _cycles = 8; }
    },
    {
      0x67,
      "LD H,A",
      [this]() { _ld8(a(), h()); _pc += 1; _cycles = 4; }
    },
    {
      0x68,
      "LD L,B",
      [this]() { _ld8(b(), l()); _pc += 1; _cycles = 4; }
    },
    {
      0x69,
      "LD L,C",
      [this]() { _ld8(c(), l()); _pc += 1; _cycles = 4; }
    },
    {
      0x6A,
      "LD L,D",
      [this]() { _ld8(d(), l()); _pc += 1; _cycles = 4; }
    },
    {
      0x6B,
      "LD L,E",
      [this]() { _ld8(e(), l()); _pc += 1; _cycles = 4; }
    },
    {
      0x6C,
      "LD L,H",
      [this]() { _ld8(h(), l()); _pc += 1; _cycles = 4; }
    },
    {
      0x6D,
      "LD L,L",
      [this]() { _ld8(l(), l()); _pc += 1; _cycles = 4; }
    },
    {
      0x6E,
      "LD L,(HL)",
      [this]() { _ld8(_mm.read(hl()), l()); _pc += 1; _cycles = 8; }
    },
    {
      0x6F,
      "LD L,A",
      [this]() { _ld8(a(), l()); _pc += 1; _cycles = 4; }
    },
    { 0x70,
      "LD (HL),B",
      [this]() { reg_t i = 0; _ld8(b(), i); _mm.write(hl(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x71,
      "LD (HL),C",
      [this]() { reg_t i = 0; _ld8(c(), i); _mm.write(hl(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x72,
      "LD (HL),D",
      [this]() { reg_t i = 0; _ld8(d(), i); _mm.write(hl(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x73,
      "LD (HL),E",
      [this]() { reg_t i = 0; _ld8(e(), i); _mm.write(hl(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x74,
      "LD (HL),H",
      [this]() { reg_t i = 0; _ld8(h(), i); _mm.write(hl(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x75,
      "LD (HL),L",
      [this]() { reg_t i = 0; _ld8(l(), i); _mm.write(hl(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x76,
      "HALT",
      [this]() { _halted = true; _pc += 1; _cycles = 4; }
    },
    {
      0x77,
      "LD (HL),A",
      [this]() { reg_t i = 0; _ld8(a(), i); _mm.write(hl(), i); _pc += 1; _cycles = 8; }
    },
    {
      0x78,
      "LD A,B",
      [this]() { _ld8(b(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x79,
      "LD A,C",
      [this]() { _ld8(c(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x7A,
      "LD A,D",
      [this]() { _ld8(d(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x7B,
      "LD A,E",
      [this]() { _ld8(e(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x7C,
      "LD A,H",
      [this]() { _ld8(h(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x7D,
      "LD A,L",
      [this]() { _ld8(l(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x7E,
      "LD A,(HL)",
      [this]() { _ld8(_mm.read(hl()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0x7F,
      "LD A,A",
      [this]() { _ld8(a(), a()); _pc += 1; _cycles = 4; }
    },

    { 0x80,
      "ADD A,B",
      [this]() { _add8(b(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x81,
      "ADD A,C",
      [this]() { _add8(c(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x82,
      "ADD A,D",
      [this]() { _add8(d(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x83,
      "ADD A,E",
      [this]() { _add8(e(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x84,
      "ADD A,H",
      [this]() { _add8(h(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x85,
      "ADD A,L",
      [this]() { _add8(l(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x86,
      "ADD A,(HL)",
      [this]() { _add8(_mm.read(hl()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0x87,
      "ADD A,A",
      [this]() { _add8(a(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x88,
      "ADC A,B",
      [this]() { _adc8(b(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x89,
      "ADC A,C",
      [this]() { _adc8(c(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x8A,
      "ADC A,D",
      [this]() { _adc8(d(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x8B,
      "ADC A,E",
      [this]() { _adc8(e(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x8C,
      "ADC A,H",
      [this]() { _adc8(h(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x8D,
      "ADC A,L",
      [this]() { _adc8(l(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x8E,
      "ADC A,(HL)",
      [this]() { _adc8(_mm.read(hl()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0x8F,
      "ADC A,A",
      [this]() { _adc8(a(), a()); _pc += 1; _cycles = 4; }
    },
    { 0x90,
      "SUB B",
      [this]() { _sub8(b(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x91,
      "SUB C",
      [this]() { _sub8(c(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x92,
      "SUB D",
      [this]() { _sub8(d(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x93,
      "SUB E",
      [this]() { _sub8(e(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x94,
      "SUB H",
      [this]() { _sub8(h(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x95,
      "SUB L",
      [this]() { _sub8(l(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x96,
      "SUB (HL)",
      [this]() { _sub8(_mm.read(hl()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0x97,
      "SUB A",
      [this]() { _sub8(a(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x98,
      "SBC A,B",
      [this]() { _sbc8(b(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x99,
      "SBC A,C",
      [this]() { _sbc8(c(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x9A,
      "SBC A,D",
      [this]() { _sbc8(d(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x9B,
      "SBC A,E",
      [this]() { _sbc8(e(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x9C,
      "SBC A,H",
      [this]() { _sbc8(h(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x9D,
      "SBC A,L",
      [this]() { _sbc8(l(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0x9E,
      "SBC A,(HL)",
      [this]() { _sbc8(_mm.read(hl()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0x9F,
      "SBC A,A",
      [this]() { _sbc8(a(), a()); _pc += 1; _cycles = 4; }
    },
    { 0xA0,
      "AND B",
      [this]() { _and(b(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xA1,
      "AND C",
      [this]() { _and(c(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xA2,
      "AND D",
      [this]() { _and(d(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xA3,
      "AND E",
      [this]() { _and(e(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xA4,
      "AND H",
      [this]() { _and(h(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xA5,
      "AND L",
      [this]() { _and(l(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xA6,
      "AND (HL)",
      [this]() { _and(_mm.read(hl()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0xA7,
      "AND A",
      [this]() { _and(a(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xA8,
      "XOR B",
      [this]() { _xor(b(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xA9,
      "XOR C",
      [this]() { _xor(c(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xAA,
      "XOR D",
      [this]() { _xor(d(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xAB,
      "XOR E",
      [this]() { _xor(e(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xAC,
      "XOR H",
      [this]() { _xor(h(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xAD,
      "XOR L",
      [this]() { _xor(l(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xAE,
      "XOR (HL)",
      [this]() { _xor(_mm.read(hl()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0xAF,
      "XOR A",
      [this]() { _xor(a(), a()); _pc += 1; _cycles = 4; }
    },

    { 0xB0,
      "OR B",
      [this]() { _or(b(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xB1,
      "OR C",
      [this]() { _or(c(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xB2,
      "OR D",
      [this]() { _or(d(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xB3,
      "OR E",
      [this]() { _or(e(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xB4,
      "OR H",
      [this]() { _or(h(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xB5,
      "OR L",
      [this]() { _or(l(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xB6,
      "OR (HL)",
      [this]() { _or(_mm.read(hl()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0xB7,
      "OR A",
      [this]() { _or(a(), a()); _pc += 1; _cycles = 4; }
    },
    {
      0xB8,
      "CP B",
      [this]() { _cp(b()); _pc += 1; _cycles = 4; }
    },
    {
      0xB9,
      "CP C",
      [this]() { _cp(c()); _pc += 1; _cycles = 4; }
    },
    {
      0xBA,
      "CP D",
      [this]() { _cp(d()); _pc += 1; _cycles = 4; }
    },
    {
      0xBB,
      "CP E",
      [this]() { _cp(e()); _pc += 1; _cycles = 4; }
    },
    {
      0xBC,
      "CP H",
      [this]() { _cp(h()); _pc += 1; _cycles = 4; }
    },
    {
      0xBD,
      "CP L",
      [this]() { _cp(l()); _pc += 1; _cycles = 4; }
    },
    {
      0xBE,
      "CP (HL)",
      [this]() { _cp(_mm.read(hl())); _pc += 1; _cycles = 8; }
    },
    {
      0xBF,
      "CP A",
      [this]() { _cp(a()); _pc += 1; _cycles = 4; }
    },

    { 0xC0,
      "RET NZ",
      [this]() { if (not zero_flag())  _pc = _pop(); else _pc += 1; _cycles = 12; }
    },
    {
      0xC1,
      "POP BC",
      [this]() { bc(_pop()); _pc += 1; _cycles = 12; }
    },
    {
      0xC2,
      "JP NZ,nn",
      [this]() { _pc = (not zero_flag()) ? nn() : _pc + 3; _cycles = 12; }
    },
    {
      0xC3,
      "JP nn",
      [this]() { _pc = nn(); _cycles = 12; }
    },
    {
      0xC4,
      "CALL NZ,nn",
      [this]() { if (not zero_flag())  _call(nn()); else _pc += 3; _cycles = 12; }
    },
    {
      0xC5,
      "PUSH BC",
      [this]() { _push(bc()); _pc += 1; _cycles = 16; }
    },
    {
      0xC6,
      "ADD A,#",
      [this]() { _add8(b1(), a()); _pc += 2; _cycles = 8; }
    },
    {
      0xC7,
      "RST 00H",
      [this]() { _call(0x0000, 1); _cycles = 32; }
    },
    {
      0xC8,
      "RET Z",
      [this]() { if (zero_flag())      _pc = _pop(); else _pc += 1; _cycles = 12; }
    },
    {
      0xC9,
      "RET",
      [this]() { _pc = _pop(); _cycles = 8; }
    },
    {
      0xCA,
      "JP Z,nn",
      [this]() { _pc = (zero_flag()) ? nn() : _pc + 3; _cycles = 12; }
    },
    {
      0xCB,
      "PREF",
      []() {}
    },
    {
      0xCC,
      "CALL Z,nn",
      [this]() { if (zero_flag())      _call(nn()); else _pc += 3; _cycles = 12; }
    },
    {
      0xCD,
      "CALL nn",
      [this]() { _call(nn()); _cycles = 12; }
    },
    {
      0xCE,
      "ADC A,#",
      [this]() { _adc8(b1(), a()); _pc += 2; _cycles = 8; }
    },
    {
      0xCF,
      "RST 08H",
      [this]() { _call(0x0008, 1); _cycles = 32; }
    },

    { 0xD0,
      "RET NC",
      [this]() { if (not carry_flag()) _pc = _pop(); else _pc += 1; _cycles = 12; }
    },
    {
      0xD1,
      "POP DE",
      [this]() { de(_pop()); _pc += 1; _cycles = 12; }
    },
    {
      0xD2,
      "JP NC,nn",
      [this]() { _pc = (not carry_flag()) ? nn() : _pc + 3; _cycles = 12; }
    },
    {
      0xD3,
      "---",
      []() {}
    },
    {
      0xD4,
      "CALL NC,nn",
      [this]() { if (not carry_flag()) _call(nn()); else _pc += 3; _cycles = 12; }
    },
    {
      0xD5,
      "PUSH DE",
      [this]() { _push(de()); _pc += 1; _cycles = 16; }
    },
    {
      0xD6,
      "SUB #",
      [this]() { _sub8(b1(), a()); _pc += 2; _cycles = 8; }
    },
    {
      0xD7,
      "RST 10H",
      [this]() { _call(0x0010, 1); _cycles = 32; }
    },
    {
      0xD8,
      "RET C",
      [this]() { if (carry_flag())     _pc = _pop(); else _pc += 1; _cycles = 12; }
    },
    {
      0xD9,
      "RETI",
      [this]() { _pc = _pop(); _ime = true; _cycles = 8; }
    },
    {
      0xDA,
      "JP C,nn",
      [this]() { _pc = (carry_flag()) ? nn() : _pc + 3; _cycles = 12; }
    },
    {
      0xDB,
      "---",
      []() {}
    },
    {
      0xDC,
      "CALL C,nn",
      [this]() { if (carry_flag())     _call(nn()); else _pc += 3; _cycles = 12; }
    },
    {
      0xDD,
      "---",
      []() {}
    },
    {
      0xDE,
      "SBC A,#",
      [this]() { _sbc8(b1(), a()); _pc += 2; _cycles = 4; }
    },
    {
      0xDF,
      "RST 18H",
      [this]() { _call(0x0018, 1); _cycles = 32; }
    },

    { 0xE0,
      "LD (n),A",
      [this]() { reg_t i = 0; _ld8(a(), i); _mm.write(0xFF00 + b1(), i); _pc += 2; _cycles = 12; }
    },
    {
      0xE1,
      "POP HL",
      [this]() { hl(_pop()); _pc += 1; _cycles = 12; }
    },
    {
      0xE2,
      "LD (C),A",
      [this]() { reg_t i = 0; _ld8(a(), i); _mm.write(0xFF00 + c(), i); _pc += 1; _cycles = 8; }
    },
    {
      0xE3,
      "---",
      []() {}
    },
    {
      0xE4,
      "---",
      []() {}
    },
    {
      0xE5,
      "PUSH HL",
      [this]() { _push(hl()); _pc += 1; _cycles = 16; }
    },
    {
      0xE6,
      "AND #",
      [this]() { _and(b1(), a()); _pc += 2; _cycles = 8; }
    },
    {
      0xE7,
      "RST 20H",
      [this]() { _call(0x0020, 1); _cycles = 32; }
    },
    {
      0xE8,
      "ADD SP,#",
      [this]()
      {
        // int arg = b1();
        // int src = sp();
        // int result = src + arg;

        // half_carry_flag(((sp() & 0x00FF) + (arg & 0x00FF)) > 0x00FF);
        // carry_flag(result > 0xFFFF);

        // FIXME SHAMELESS COPY
        wide_reg_t reg = sp();
        int8_t   value = b1();

        int result = static_cast<int>(reg + value);

        zero_flag(false);
        substract_flag(false);
        half_carry_flag(((reg ^ value ^ (result & 0xFFFF)) & 0x10) == 0x10);
        carry_flag(((reg ^ value ^ (result & 0xFFFF)) & 0x100) == 0x100);

        sp(static_cast<wide_reg_t>(result));

        // sp(_add16(b1(), sp()));

        // substract_flag(false);
        // zero_flag(false);
        _pc += 2;
        _cycles = 16;
      }
    },
    {
      0xE9,
      "JP (HL)",
      [this]() { _pc = hl(); _cycles = 4; }
    },
    {
      0xEA,
      "LD (nn),A",
      [this]() { reg_t i = 0; _ld8(a(), i); _mm.write(nn(), i); _pc += 3; _cycles = 8; }
    },
    {
      0xEB,
      "---",
      []() {}
    },
    {
      0xEC,
      "---",
      []() {}
    },
    {
      0xED,
      "---",
      []() {}
    },
    {
      0xEE,
      "XOR #",
      [this]() { _xor(b1(), a()); _pc += 2; _cycles = 8; }
    },
    {
      0xEF,
      "RST 28H",
      [this]() { _call(0x0028, 1); _cycles = 32; }
    },
    { 0xF0,
      "LD A,(n)",
      [this]() { _ld8(_mm.read(0xFF00 + b1()), a()); _pc += 2; _cycles = 12; }
    },
    {
      0xF1,
      "POP AF",
      [this]() { af(_pop()); _pc += 1; _cycles = 12; }
    },
    {
      0xF2,
      "LD A,(C)",
      [this]() { _ld8(_mm.read(0xFF00 + c()), a()); _pc += 1; _cycles = 8; }
    },
    {
      0xF3,
      "DI",
      [this]() { _ime = false; _pc += 1; _cycles = 4; }
    },
    {
      0xF4,
      "---",
      []() {}
    },
    {
      0xF5,
      "PUSH AF",
      [this]() { _push(af()); _pc += 1; _cycles = 16; }
    },
    {
      0xF6,
      "OR #",
      [this]() { _or(b1(), a()); _pc += 2; _cycles = 8; }
    },
    {
      0xF7,
      "RST 30H",
      [this]() { _call(0x0030, 1); _cycles = 32; }
    },
    {
      0xF8,
      "LD HL,SP+n",
      [this]()
      {
        // FIXME SHAMELESS COPY
        // _ld_hl_spn(b1());
        wide_reg_t reg = sp();
        int8_t   value = b1();

        int result = static_cast<int>(reg + value);

        zero_flag(false);
        substract_flag(false);
        half_carry_flag(((reg ^ value ^ (result & 0xFFFF)) & 0x10) == 0x10);
        carry_flag(((reg ^ value ^ (result & 0xFFFF)) & 0x100) == 0x100);

        hl(static_cast<wide_reg_t>(result));
        _pc += 2;
        _cycles = 12;
      }
    },
    {
      0xF9,
      "LD SP,HL",
      [this]() { sp(hl()); _pc += 1; _cycles = 8; }
    },
    {
      0xFA,
      "LD A,(nn)",
      [this]() { _ld8(_mm.read(nn()), a()); _pc += 3; _cycles = 16; }
    },
    {
      0xFB,
      "EI",
      [this]() { _ime = true; _pc += 1; _cycles = 4; }
    },
    {
      0xFC,
      "---",
      []() {}
    },
    {
      0xFD,
      "---",
      []() {}
    },
    {
      0xFE,
      "CP #",
      [this]() { _cp(b1()); _pc += 2; _cycles = 8; }
    },
    {
      0xFF,
      "RST 38H",
      [this]() { _call(0x0038, 1); _cycles = 32; }
    }
  }};

  std::map<reg_t, std::function<void()>> _pref {
    { 0x00, /* RLC B   */ [this]() { _rlc(b(), true); _pc += 2; _cycles = 8; } },
    { 0x01, /* RLC C   */ [this]() { _rlc(c(), true); _pc += 2; _cycles = 8; } },
    { 0x02, /* RLC D   */ [this]() { _rlc(d(), true); _pc += 2; _cycles = 8; } },
    { 0x03, /* RLC E   */ [this]() { _rlc(e(), true); _pc += 2; _cycles = 8; } },
    { 0x04, /* RLC H   */ [this]() { _rlc(h(), true); _pc += 2; _cycles = 8; } },
    { 0x05, /* RLC L   */ [this]() { _rlc(l(), true); _pc += 2; _cycles = 8; } },
    { 0x06, /* RLC (HL)*/ [this]() { reg_t i = _mm.read(hl()); _rlc(i, true); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x07, /* RLC A   */ [this]() { _rlc(a(), true); _pc += 2; _cycles = 8; } },

    { 0x08, /* RRC B   */ [this]() { _rrc(b(), true); _pc += 2; _cycles = 8; } },
    { 0x09, /* RRC C   */ [this]() { _rrc(c(), true); _pc += 2; _cycles = 8; } },
    { 0x0A, /* RRC D   */ [this]() { _rrc(d(), true); _pc += 2; _cycles = 8; } },
    { 0x0B, /* RRC E   */ [this]() { _rrc(e(), true); _pc += 2; _cycles = 8; } },
    { 0x0C, /* RRC H   */ [this]() { _rrc(h(), true); _pc += 2; _cycles = 8; } },
    { 0x0D, /* RRC L   */ [this]() { _rrc(l(), true); _pc += 2; _cycles = 8; } },
    { 0x0E, /* RRC (HL)*/ [this]() { reg_t i = _mm.read(hl()); _rrc(i, true); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x0F, /* RRC A   */ [this]() { _rrc(a(), true); _pc += 2; _cycles = 8; } },

    { 0x10, /* RL B    */ [this]() { _rl(b(), true); _pc += 2; _cycles = 8; } },
    { 0x11, /* RL C    */ [this]() { _rl(c(), true); _pc += 2; _cycles = 8; } },
    { 0x12, /* RL D    */ [this]() { _rl(d(), true); _pc += 2; _cycles = 8; } },
    { 0x13, /* RL E    */ [this]() { _rl(e(), true); _pc += 2; _cycles = 8; } },
    { 0x14, /* RL H    */ [this]() { _rl(h(), true); _pc += 2; _cycles = 8; } },
    { 0x15, /* RL L    */ [this]() { _rl(l(), true); _pc += 2; _cycles = 8; } },
    { 0x16, /* RL (HL) */ [this]() { reg_t i = _mm.read(hl()); _rl(i, true); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x17, /* RL A    */ [this]() { _rl(a(), true); _pc += 2; _cycles = 8; } },

    { 0x18, /* RR B    */ [this]() { _rr(b(), true); _pc += 2; _cycles = 8; } },
    { 0x19, /* RR C    */ [this]() { _rr(c(), true); _pc += 2; _cycles = 8; } },
    { 0x1A, /* RR D    */ [this]() { _rr(d(), true); _pc += 2; _cycles = 8; } },
    { 0x1B, /* RR E    */ [this]() { _rr(e(), true); _pc += 2; _cycles = 8; } },
    { 0x1C, /* RR H    */ [this]() { _rr(h(), true); _pc += 2; _cycles = 8; } },
    { 0x1D, /* RR L    */ [this]() { _rr(l(), true); _pc += 2; _cycles = 8; } },
    { 0x1E, /* RR (HL) */ [this]() { reg_t i = _mm.read(hl()); _rr(i, true); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x1F, /* RR A    */ [this]() { _rr(a(), true); _pc += 2; _cycles = 8; } },

    { 0x20, /* SLA B    */ [this]() { _sla(b()); _pc += 2; _cycles = 8; } },
    { 0x21, /* SLA C    */ [this]() { _sla(c()); _pc += 2; _cycles = 8; } },
    { 0x22, /* SLA D    */ [this]() { _sla(d()); _pc += 2; _cycles = 8; } },
    { 0x23, /* SLA E    */ [this]() { _sla(e()); _pc += 2; _cycles = 8; } },
    { 0x24, /* SLA H    */ [this]() { _sla(h()); _pc += 2; _cycles = 8; } },
    { 0x25, /* SLA L    */ [this]() { _sla(l()); _pc += 2; _cycles = 8; } },
    { 0x26, /* SLA (HL) */ [this]() { reg_t i = _mm.read(hl()); _sla(i); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x27, /* SLA A    */ [this]() { _sla(a()); _pc += 2; _cycles = 8; } },

    { 0x28, /* SRA B    */ [this]() { _sra(b()); _pc += 2; _cycles = 8; } },
    { 0x29, /* SRA C    */ [this]() { _sra(c()); _pc += 2; _cycles = 8; } },
    { 0x2A, /* SRA D    */ [this]() { _sra(d()); _pc += 2; _cycles = 8; } },
    { 0x2B, /* SRA E    */ [this]() { _sra(e()); _pc += 2; _cycles = 8; } },
    { 0x2C, /* SRA H    */ [this]() { _sra(h()); _pc += 2; _cycles = 8; } },
    { 0x2D, /* SRA L    */ [this]() { _sra(l()); _pc += 2; _cycles = 8; } },
    { 0x2E, /* SRA (HL) */ [this]() { reg_t i = _mm.read(hl()); _sra(i); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x2F, /* SRA A    */ [this]() { _sra(a()); _pc += 2; _cycles = 8; } },

    { 0x30, /* SWAP B  */ [this]() { _swap(b()); _pc += 2; _cycles = 8; } },
    { 0x31, /* SWAP C  */ [this]() { _swap(c()); _pc += 2; _cycles = 8; } },
    { 0x32, /* SWAP D  */ [this]() { _swap(d()); _pc += 2; _cycles = 8; } },
    { 0x33, /* SWAP E  */ [this]() { _swap(e()); _pc += 2; _cycles = 8; } },
    { 0x34, /* SWAP H  */ [this]() { _swap(h()); _pc += 2; _cycles = 8; } },
    { 0x35, /* SWAP L  */ [this]() { _swap(l()); _pc += 2; _cycles = 8; } },
    { 0x36, /* SWAP(HL)*/ [this]() { reg_t i = _mm.read(hl()); _swap(i); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x37, /* SWAP A  */ [this]() { _swap(a()); _pc += 2; _cycles = 8; } },

    { 0x38, /* SRL B    */ [this]() { _srl(b()); _pc += 2; _cycles = 8; } },
    { 0x39, /* SRL C    */ [this]() { _srl(c()); _pc += 2; _cycles = 8; } },
    { 0x3A, /* SRL D    */ [this]() { _srl(d()); _pc += 2; _cycles = 8; } },
    { 0x3B, /* SRL E    */ [this]() { _srl(e()); _pc += 2; _cycles = 8; } },
    { 0x3C, /* SRL H    */ [this]() { _srl(h()); _pc += 2; _cycles = 8; } },
    { 0x3D, /* SRL L    */ [this]() { _srl(l()); _pc += 2; _cycles = 8; } },
    { 0x3E, /* SRL (HL) */ [this]() { reg_t i = _mm.read(hl()); _srl(i); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x3F, /* SRL A    */ [this]() { _srl(a()); _pc += 2; _cycles = 8; } },

    { 0x40, /* BIT 0,B    */ [this]() { _bit(b(), 0); _pc += 2; _cycles = 8; } },
    { 0x41, /* BIT 0,C    */ [this]() { _bit(c(), 0); _pc += 2; _cycles = 8; } },
    { 0x42, /* BIT 0,D    */ [this]() { _bit(d(), 0); _pc += 2; _cycles = 8; } },
    { 0x43, /* BIT 0,E    */ [this]() { _bit(e(), 0); _pc += 2; _cycles = 8; } },
    { 0x44, /* BIT 0,H    */ [this]() { _bit(h(), 0); _pc += 2; _cycles = 8; } },
    { 0x45, /* BIT 0,L    */ [this]() { _bit(l(), 0); _pc += 2; _cycles = 8; } },
    { 0x46, /* BIT 0,(HL) */ [this]() { reg_t i = _mm.read(hl()); _bit(i, 0); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x47, /* BIT 0,A    */ [this]() { _bit(a(), 0); _pc += 2; _cycles = 8; } },

    { 0x48, /* BIT 1,B    */ [this]() { _bit(b(), 1); _pc += 2; _cycles = 8; } },
    { 0x49, /* BIT 1,C    */ [this]() { _bit(c(), 1); _pc += 2; _cycles = 8; } },
    { 0x4A, /* BIT 1,D    */ [this]() { _bit(d(), 1); _pc += 2; _cycles = 8; } },
    { 0x4B, /* BIT 1,E    */ [this]() { _bit(e(), 1); _pc += 2; _cycles = 8; } },
    { 0x4C, /* BIT 1,H    */ [this]() { _bit(h(), 1); _pc += 2; _cycles = 8; } },
    { 0x4D, /* BIT 1,L    */ [this]() { _bit(l(), 1); _pc += 2; _cycles = 8; } },
    { 0x4E, /* BIT 1,(HL) */ [this]() { reg_t i = _mm.read(hl()); _bit(i, 1); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x4F, /* BIT 1,A    */ [this]() { _bit(a(), 1); _pc += 2; _cycles = 8; } },

    { 0x50, /* BIT 2,B    */ [this]() { _bit(b(), 2); _pc += 2; _cycles = 8; } },
    { 0x51, /* BIT 2,C    */ [this]() { _bit(c(), 2); _pc += 2; _cycles = 8; } },
    { 0x52, /* BIT 2,D    */ [this]() { _bit(d(), 2); _pc += 2; _cycles = 8; } },
    { 0x53, /* BIT 2,E    */ [this]() { _bit(e(), 2); _pc += 2; _cycles = 8; } },
    { 0x54, /* BIT 2,H    */ [this]() { _bit(h(), 2); _pc += 2; _cycles = 8; } },
    { 0x55, /* BIT 2,L    */ [this]() { _bit(l(), 2); _pc += 2; _cycles = 8; } },
    { 0x56, /* BIT 2,(HL) */ [this]() { reg_t i = _mm.read(hl()); _bit(i, 2); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x57, /* BIT 2,A    */ [this]() { _bit(a(), 2); _pc += 2; _cycles = 8; } },

    { 0x58, /* BIT 3,B    */ [this]() { _bit(b(), 3); _pc += 2; _cycles = 8; } },
    { 0x59, /* BIT 3,C    */ [this]() { _bit(c(), 3); _pc += 2; _cycles = 8; } },
    { 0x5A, /* BIT 3,D    */ [this]() { _bit(d(), 3); _pc += 2; _cycles = 8; } },
    { 0x5B, /* BIT 3,E    */ [this]() { _bit(e(), 3); _pc += 2; _cycles = 8; } },
    { 0x5C, /* BIT 3,H    */ [this]() { _bit(h(), 3); _pc += 2; _cycles = 8; } },
    { 0x5D, /* BIT 3,L    */ [this]() { _bit(l(), 3); _pc += 2; _cycles = 8; } },
    { 0x5E, /* BIT 3,(HL) */ [this]() { _bit(_mm.read(hl()), 3); _pc += 2; _cycles = 16; } },
    { 0x5F, /* BIT 3,A    */ [this]() { _bit(a(), 3); _pc += 2; _cycles = 8; } },

    { 0x60, /* BIT 4,B    */ [this]() { _bit(b(), 4); _pc += 2; _cycles = 8; } },
    { 0x61, /* BIT 4,C    */ [this]() { _bit(c(), 4); _pc += 2; _cycles = 8; } },
    { 0x62, /* BIT 4,D    */ [this]() { _bit(d(), 4); _pc += 2; _cycles = 8; } },
    { 0x63, /* BIT 4,E    */ [this]() { _bit(e(), 4); _pc += 2; _cycles = 8; } },
    { 0x64, /* BIT 4,H    */ [this]() { _bit(h(), 4); _pc += 2; _cycles = 8; } },
    { 0x65, /* BIT 4,L    */ [this]() { _bit(l(), 4); _pc += 2; _cycles = 8; } },
    { 0x66, /* BIT 4,(HL) */ [this]() { _bit(_mm.read(hl()), 4); _pc += 2; _cycles = 16; } },
    { 0x67, /* BIT 4,A    */ [this]() { _bit(a(), 4); _pc += 2; _cycles = 8; } },

    { 0x68, /* BIT 5,B    */ [this]() { _bit(b(), 5); _pc += 2; _cycles = 8; } },
    { 0x69, /* BIT 5,C    */ [this]() { _bit(c(), 5); _pc += 2; _cycles = 8; } },
    { 0x6A, /* BIT 5,D    */ [this]() { _bit(d(), 5); _pc += 2; _cycles = 8; } },
    { 0x6B, /* BIT 5,E    */ [this]() { _bit(e(), 5); _pc += 2; _cycles = 8; } },
    { 0x6C, /* BIT 5,H    */ [this]() { _bit(h(), 5); _pc += 2; _cycles = 8; } },
    { 0x6D, /* BIT 5,L    */ [this]() { _bit(l(), 5); _pc += 2; _cycles = 8; } },
    { 0x6E, /* BIT 5,(HL) */ [this]() { _bit(_mm.read(hl()), 5); _pc += 2; _cycles = 16; } },
    { 0x6F, /* BIT 5,A    */ [this]() { _bit(a(), 5); _pc += 2; _cycles = 8; } },

    { 0x70, /* BIT 6,B    */ [this]() { _bit(b(), 6); _pc += 2; _cycles = 8; } },
    { 0x71, /* BIT 6,C    */ [this]() { _bit(c(), 6); _pc += 2; _cycles = 8; } },
    { 0x72, /* BIT 6,D    */ [this]() { _bit(d(), 6); _pc += 2; _cycles = 8; } },
    { 0x73, /* BIT 6,E    */ [this]() { _bit(e(), 6); _pc += 2; _cycles = 8; } },
    { 0x74, /* BIT 6,H    */ [this]() { _bit(h(), 6); _pc += 2; _cycles = 8; } },
    { 0x75, /* BIT 6,L    */ [this]() { _bit(l(), 6); _pc += 2; _cycles = 8; } },
    { 0x76, /* BIT 6,(HL) */ [this]() { _bit(_mm.read(hl()), 6); _pc += 2; _cycles = 16; } },
    { 0x77, /* BIT 6,A    */ [this]() { _bit(a(), 6); _pc += 2; _cycles = 8; } },

    { 0x78, /* BIT 7,B    */ [this]() { _bit(b(), 7); _pc += 2; _cycles = 8; } },
    { 0x79, /* BIT 7,C    */ [this]() { _bit(c(), 7); _pc += 2; _cycles = 8; } },
    { 0x7A, /* BIT 7,D    */ [this]() { _bit(d(), 7); _pc += 2; _cycles = 8; } },
    { 0x7B, /* BIT 7,E    */ [this]() { _bit(e(), 7); _pc += 2; _cycles = 8; } },
    { 0x7C, /* BIT 7,H    */ [this]() { _bit(h(), 7); _pc += 2; _cycles = 8; } },
    { 0x7D, /* BIT 7,L    */ [this]() { _bit(l(), 7); _pc += 2; _cycles = 8; } },
    { 0x7E, /* BIT 7,(HL) */ [this]() { _bit(_mm.read(hl()), 7); _pc += 2; _cycles = 16; } },
    { 0x7F, /* BIT 7,A    */ [this]() { _bit(a(), 7); _pc += 2; _cycles = 8; } },

    { 0x80, /* RES 0,B    */ [this]() { _res(b(), 0); _pc += 2; _cycles = 8; } },
    { 0x81, /* RES 0,C    */ [this]() { _res(c(), 0); _pc += 2; _cycles = 8; } },
    { 0x82, /* RES 0,D    */ [this]() { _res(d(), 0); _pc += 2; _cycles = 8; } },
    { 0x83, /* RES 0,E    */ [this]() { _res(e(), 0); _pc += 2; _cycles = 8; } },
    { 0x84, /* RES 0,H    */ [this]() { _res(h(), 0); _pc += 2; _cycles = 8; } },
    { 0x85, /* RES 0,L    */ [this]() { _res(l(), 0); _pc += 2; _cycles = 8; } },
    { 0x86, /* RES 0,(HL) */ [this]() { reg_t i = _mm.read(hl()); _res(i, 0); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x87, /* RES 0,A    */ [this]() { _res(a(), 0); _pc += 2; _cycles = 8; } },

    { 0x88, /* RES 1,B    */ [this]() { _res(b(), 1); _pc += 2; _cycles = 8; } },
    { 0x89, /* RES 1,C    */ [this]() { _res(c(), 1); _pc += 2; _cycles = 8; } },
    { 0x8A, /* RES 1,D    */ [this]() { _res(d(), 1); _pc += 2; _cycles = 8; } },
    { 0x8B, /* RES 1,E    */ [this]() { _res(e(), 1); _pc += 2; _cycles = 8; } },
    { 0x8C, /* RES 1,H    */ [this]() { _res(h(), 1); _pc += 2; _cycles = 8; } },
    { 0x8D, /* RES 1,L    */ [this]() { _res(l(), 1); _pc += 2; _cycles = 8; } },
    { 0x8E, /* RES 1,(HL) */ [this]() { reg_t i = _mm.read(hl()); _res(i, 1); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x8F, /* RES 1,A    */ [this]() { _res(a(), 1); _pc += 2; _cycles = 8; } },

    { 0x90, /* RES 2,B    */ [this]() { _res(b(), 2); _pc += 2; _cycles = 8; } },
    { 0x91, /* RES 2,C    */ [this]() { _res(c(), 2); _pc += 2; _cycles = 8; } },
    { 0x92, /* RES 2,D    */ [this]() { _res(d(), 2); _pc += 2; _cycles = 8; } },
    { 0x93, /* RES 2,E    */ [this]() { _res(e(), 2); _pc += 2; _cycles = 8; } },
    { 0x94, /* RES 2,H    */ [this]() { _res(h(), 2); _pc += 2; _cycles = 8; } },
    { 0x95, /* RES 2,L    */ [this]() { _res(l(), 2); _pc += 2; _cycles = 8; } },
    { 0x96, /* RES 2,(HL) */ [this]() { reg_t i = _mm.read(hl()); _res(i, 2); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x97, /* RES 2,A    */ [this]() { _res(a(), 2); _pc += 2; _cycles = 8; } },

    { 0x98, /* RES 3,B    */ [this]() { _res(b(), 3); _pc += 2; _cycles = 8; } },
    { 0x99, /* RES 3,C    */ [this]() { _res(c(), 3); _pc += 2; _cycles = 8; } },
    { 0x9A, /* RES 3,D    */ [this]() { _res(d(), 3); _pc += 2; _cycles = 8; } },
    { 0x9B, /* RES 3,E    */ [this]() { _res(e(), 3); _pc += 2; _cycles = 8; } },
    { 0x9C, /* RES 3,H    */ [this]() { _res(h(), 3); _pc += 2; _cycles = 8; } },
    { 0x9D, /* RES 3,L    */ [this]() { _res(l(), 3); _pc += 2; _cycles = 8; } },
    { 0x9E, /* RES 3,(HL) */ [this]() { reg_t i = _mm.read(hl()); _res(i, 3); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0x9F, /* RES 3,A    */ [this]() { _res(a(), 3); _pc += 2; _cycles = 8; } },

    { 0xA0, /* RES 4,B    */ [this]() { _res(b(), 4); _pc += 2; _cycles = 8; } },
    { 0xA1, /* RES 4,C    */ [this]() { _res(c(), 4); _pc += 2; _cycles = 8; } },
    { 0xA2, /* RES 4,D    */ [this]() { _res(d(), 4); _pc += 2; _cycles = 8; } },
    { 0xA3, /* RES 4,E    */ [this]() { _res(e(), 4); _pc += 2; _cycles = 8; } },
    { 0xA4, /* RES 4,H    */ [this]() { _res(h(), 4); _pc += 2; _cycles = 8; } },
    { 0xA5, /* RES 4,L    */ [this]() { _res(l(), 4); _pc += 2; _cycles = 8; } },
    { 0xA6, /* RES 4,(HL) */ [this]() { reg_t i = _mm.read(hl()); _res(i, 4); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xA7, /* RES 4,A    */ [this]() { _res(a(), 4); _pc += 2; _cycles = 8; } },

    { 0xA8, /* RES 5,B    */ [this]() { _res(b(), 5); _pc += 2; _cycles = 8; } },
    { 0xA9, /* RES 5,C    */ [this]() { _res(c(), 5); _pc += 2; _cycles = 8; } },
    { 0xAA, /* RES 5,D    */ [this]() { _res(d(), 5); _pc += 2; _cycles = 8; } },
    { 0xAB, /* RES 5,E    */ [this]() { _res(e(), 5); _pc += 2; _cycles = 8; } },
    { 0xAC, /* RES 5,H    */ [this]() { _res(h(), 5); _pc += 2; _cycles = 8; } },
    { 0xAD, /* RES 5,L    */ [this]() { _res(l(), 5); _pc += 2; _cycles = 8; } },
    { 0xAE, /* RES 5,(HL) */ [this]() { reg_t i = _mm.read(hl()); _res(i, 5); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xAF, /* RES 5,A    */ [this]() { _res(a(), 5); _pc += 2; _cycles = 8; } },

    { 0xB0, /* RES 6,B    */ [this]() { _res(b(), 6); _pc += 2; _cycles = 8; } },
    { 0xB1, /* RES 6,C    */ [this]() { _res(c(), 6); _pc += 2; _cycles = 8; } },
    { 0xB2, /* RES 6,D    */ [this]() { _res(d(), 6); _pc += 2; _cycles = 8; } },
    { 0xB3, /* RES 6,E    */ [this]() { _res(e(), 6); _pc += 2; _cycles = 8; } },
    { 0xB4, /* RES 6,H    */ [this]() { _res(h(), 6); _pc += 2; _cycles = 8; } },
    { 0xB5, /* RES 6,L    */ [this]() { _res(l(), 6); _pc += 2; _cycles = 8; } },
    { 0xB6, /* RES 6,(HL) */ [this]() { reg_t i = _mm.read(hl()); _res(i, 6); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xB7, /* RES 6,A    */ [this]() { _res(a(), 6); _pc += 2; _cycles = 8; } },

    { 0xB8, /* RES 7,B    */ [this]() { _res(b(), 7); _pc += 2; _cycles = 8; } },
    { 0xB9, /* RES 7,C    */ [this]() { _res(c(), 7); _pc += 2; _cycles = 8; } },
    { 0xBA, /* RES 7,D    */ [this]() { _res(d(), 7); _pc += 2; _cycles = 8; } },
    { 0xBB, /* RES 7,E    */ [this]() { _res(e(), 7); _pc += 2; _cycles = 8; } },
    { 0xBC, /* RES 7,H    */ [this]() { _res(h(), 7); _pc += 2; _cycles = 8; } },
    { 0xBD, /* RES 7,L    */ [this]() { _res(l(), 7); _pc += 2; _cycles = 8; } },
    { 0xBE, /* RES 7,(HL) */ [this]() { reg_t i = _mm.read(hl()); _res(i, 7); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xBF, /* RES 7,A    */ [this]() { _res(a(), 7); _pc += 2; _cycles = 8; } },

    { 0xC0, /* SET 0,B    */ [this]() { _set(b(), 0); _pc += 2; _cycles = 8; } },
    { 0xC1, /* SET 0,C    */ [this]() { _set(c(), 0); _pc += 2; _cycles = 8; } },
    { 0xC2, /* SET 0,D    */ [this]() { _set(d(), 0); _pc += 2; _cycles = 8; } },
    { 0xC3, /* SET 0,E    */ [this]() { _set(e(), 0); _pc += 2; _cycles = 8; } },
    { 0xC4, /* SET 0,H    */ [this]() { _set(h(), 0); _pc += 2; _cycles = 8; } },
    { 0xC5, /* SET 0,L    */ [this]() { _set(l(), 0); _pc += 2; _cycles = 8; } },
    { 0xC6, /* SET 0,(HL) */ [this]() { reg_t i = _mm.read(hl()); _set(i, 0); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xC7, /* SET 0,A    */ [this]() { _set(a(), 0); _pc += 2; _cycles = 8; } },

    { 0xC8, /* SET 1,B    */ [this]() { _set(b(), 1); _pc += 2; _cycles = 8; } },
    { 0xC9, /* SET 1,C    */ [this]() { _set(c(), 1); _pc += 2; _cycles = 8; } },
    { 0xCA, /* SET 1,D    */ [this]() { _set(d(), 1); _pc += 2; _cycles = 8; } },
    { 0xCB, /* SET 1,E    */ [this]() { _set(e(), 1); _pc += 2; _cycles = 8; } },
    { 0xCC, /* SET 1,H    */ [this]() { _set(h(), 1); _pc += 2; _cycles = 8; } },
    { 0xCD, /* SET 1,L    */ [this]() { _set(l(), 1); _pc += 2; _cycles = 8; } },
    { 0xCE, /* SET 1,(HL) */ [this]() { reg_t i = _mm.read(hl()); _set(i, 1); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xCF, /* SET 1,A    */ [this]() { _set(a(), 1); _pc += 2; _cycles = 8; } },

    { 0xD0, /* SET 2,B    */ [this]() { _set(b(), 2); _pc += 2; _cycles = 8; } },
    { 0xD1, /* SET 2,C    */ [this]() { _set(c(), 2); _pc += 2; _cycles = 8; } },
    { 0xD2, /* SET 2,D    */ [this]() { _set(d(), 2); _pc += 2; _cycles = 8; } },
    { 0xD3, /* SET 2,E    */ [this]() { _set(e(), 2); _pc += 2; _cycles = 8; } },
    { 0xD4, /* SET 2,H    */ [this]() { _set(h(), 2); _pc += 2; _cycles = 8; } },
    { 0xD5, /* SET 2,L    */ [this]() { _set(l(), 2); _pc += 2; _cycles = 8; } },
    { 0xD6, /* SET 2,(HL) */ [this]() { reg_t i = _mm.read(hl()); _set(i, 2); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xD7, /* SET 2,A    */ [this]() { _set(a(), 2); _pc += 2; _cycles = 8; } },

    { 0xD8, /* SET 3,B    */ [this]() { _set(b(), 3); _pc += 2; _cycles = 8; } },
    { 0xD9, /* SET 3,C    */ [this]() { _set(c(), 3); _pc += 2; _cycles = 8; } },
    { 0xDA, /* SET 3,D    */ [this]() { _set(d(), 3); _pc += 2; _cycles = 8; } },
    { 0xDB, /* SET 3,E    */ [this]() { _set(e(), 3); _pc += 2; _cycles = 8; } },
    { 0xDC, /* SET 3,H    */ [this]() { _set(h(), 3); _pc += 2; _cycles = 8; } },
    { 0xDD, /* SET 3,L    */ [this]() { _set(l(), 3); _pc += 2; _cycles = 8; } },
    { 0xDE, /* SET 3,(HL) */ [this]() { reg_t i = _mm.read(hl()); _set(i, 3); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xDF, /* SET 3,A    */ [this]() { _set(a(), 3); _pc += 2; _cycles = 8; } },

    { 0xE0, /* SET 4,B    */ [this]() { _set(b(), 4); _pc += 2; _cycles = 8; } },
    { 0xE1, /* SET 4,C    */ [this]() { _set(c(), 4); _pc += 2; _cycles = 8; } },
    { 0xE2, /* SET 4,D    */ [this]() { _set(d(), 4); _pc += 2; _cycles = 8; } },
    { 0xE3, /* SET 4,E    */ [this]() { _set(e(), 4); _pc += 2; _cycles = 8; } },
    { 0xE4, /* SET 4,H    */ [this]() { _set(h(), 4); _pc += 2; _cycles = 8; } },
    { 0xE5, /* SET 4,L    */ [this]() { _set(l(), 4); _pc += 2; _cycles = 8; } },
    { 0xE6, /* SET 4,(HL) */ [this]() { reg_t i = _mm.read(hl()); _set(i, 4); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xE7, /* SET 4,A    */ [this]() { _set(a(), 4); _pc += 2; _cycles = 8; } },

    { 0xE8, /* SET 5,B    */ [this]() { _set(b(), 5); _pc += 2; _cycles = 8; } },
    { 0xE9, /* SET 5,C    */ [this]() { _set(c(), 5); _pc += 2; _cycles = 8; } },
    { 0xEA, /* SET 5,D    */ [this]() { _set(d(), 5); _pc += 2; _cycles = 8; } },
    { 0xEB, /* SET 5,E    */ [this]() { _set(e(), 5); _pc += 2; _cycles = 8; } },
    { 0xEC, /* SET 5,H    */ [this]() { _set(h(), 5); _pc += 2; _cycles = 8; } },
    { 0xED, /* SET 5,L    */ [this]() { _set(l(), 5); _pc += 2; _cycles = 8; } },
    { 0xEE, /* SET 5,(HL) */ [this]() { reg_t i = _mm.read(hl()); _set(i, 5); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xEF, /* SET 5,A    */ [this]() { _set(a(), 5); _pc += 2; _cycles = 8; } },

    { 0xF0, /* SET 6,B    */ [this]() { _set(b(), 6); _pc += 2; _cycles = 8; } },
    { 0xF1, /* SET 6,C    */ [this]() { _set(c(), 6); _pc += 2; _cycles = 8; } },
    { 0xF2, /* SET 6,D    */ [this]() { _set(d(), 6); _pc += 2; _cycles = 8; } },
    { 0xF3, /* SET 6,E    */ [this]() { _set(e(), 6); _pc += 2; _cycles = 8; } },
    { 0xF4, /* SET 6,H    */ [this]() { _set(h(), 6); _pc += 2; _cycles = 8; } },
    { 0xF5, /* SET 6,L    */ [this]() { _set(l(), 6); _pc += 2; _cycles = 8; } },
    { 0xF6, /* SET 6,(HL) */ [this]() { reg_t i = _mm.read(hl()); _set(i, 6); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xF7, /* SET 6,A    */ [this]() { _set(a(), 6); _pc += 2; _cycles = 8; } },

    { 0xF8, /* SET 7,B    */ [this]() { _set(b(), 7); _pc += 2; _cycles = 8; } },
    { 0xF9, /* SET 7,C    */ [this]() { _set(c(), 7); _pc += 2; _cycles = 8; } },
    { 0xFA, /* SET 7,D    */ [this]() { _set(d(), 7); _pc += 2; _cycles = 8; } },
    { 0xFB, /* SET 7,E    */ [this]() { _set(e(), 7); _pc += 2; _cycles = 8; } },
    { 0xFC, /* SET 7,H    */ [this]() { _set(h(), 7); _pc += 2; _cycles = 8; } },
    { 0xFD, /* SET 7,L    */ [this]() { _set(l(), 7); _pc += 2; _cycles = 8; } },
    { 0xFE, /* SET 7,(HL) */ [this]() { reg_t i = _mm.read(hl()); _set(i, 7); _mm.write(hl(), i); _pc += 2; _cycles = 16; } },
    { 0xFF, /* SET 7,A    */ [this]() { _set(a(), 7); _pc += 2; _cycles = 8; } },
  };
};
