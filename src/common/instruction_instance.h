#ifndef COMMON_INSTRUCTION_INSTANCE_H
#define COMMON_INSTRUCTION_INSTANCE_H

#include "common/types.h"
#include "common/format.h"

namespace sicxe {

// Represents a decoded instruction
struct InstructionInstance {
  struct OperandsF2 {
    uint8 r1;
    uint8 r2;
  };

  struct OperandsFS34 {
    bool n;
    bool i;
    bool x;
    bool b;
    bool p;
    bool e;
    uint32 address;
  };

  uint8 opcode;
  Format::FormatId format;
  union {
    OperandsF2 f2;
    OperandsFS34 fS34;
  } operands;
};

}  // namespace sicxe

#endif  // COMMON_INSTRUCTION_INSTANCE_H
