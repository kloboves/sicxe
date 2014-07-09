#ifndef COMMON_OPCODE_H
#define COMMON_OPCODE_H

#include "common/macros.h"

namespace sicxe {

class Opcode {
 public:
  DISALLOW_INSTANTIATE(Opcode);

  enum OpcodeId {
    CLEAR  = 0xB4,
    RMO    = 0xAC,

    ADD    = 0x18,
    SUB    = 0x1C,
    MUL    = 0x20,
    DIV    = 0x24,
    AND    = 0x40,
    OR     = 0x44,

    ADDR   = 0x90,
    SUBR   = 0x94,
    MULR   = 0x98,
    DIVR   = 0x9C,

    SHIFTL = 0xA4,
    SHIFTR = 0xA8,

    COMP   = 0x28,
    COMPR  = 0xA0,

    TIX    = 0x2C,
    TIXR   = 0xB8,

    J      = 0x3C,
    JEQ    = 0x30,
    JGT    = 0x34,
    JLT    = 0x38,
    JSUB   = 0x48,
    RSUB   = 0x4C,

    LDCH   = 0x50,
    STCH   = 0x54,
    STSW   = 0xE8,

    LDA    = 0x00,
    LDB    = 0x68,
    LDL    = 0x08,
    LDS    = 0x6C,
    LDT    = 0x74,
    LDX    = 0x04,
    STA    = 0x0C,
    STB    = 0x78,
    STL    = 0x14,
    STS    = 0x7C,
    STT    = 0x84,
    STX    = 0x10,

    TD     = 0xE0,
    RD     = 0xD8,
    WD     = 0xDC,

    FIX    = 0xC4,
    FLOAT  = 0xC0,

    ADDF   = 0x58,
    SUBF   = 0x5C,
    MULF   = 0x60,
    DIVF   = 0x64,
    COMPF  = 0x88,

    LDF    = 0x70,
    STF    = 0x80,

    // extended instruction set
    XOR    = 0xF0,
    ANDR   = 0xF4,
    ORR    = 0xF5,
    XORR   = 0xF6,
    NOT    = 0xF7,

    EINT   = 0xF8,
    DINT   = 0xF9,
    RINT   = 0xFA,
    STIL   = 0xFC,
  };
};

}  // namespace sicxe

#endif  // COMMON_OPCODE_H
