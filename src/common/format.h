#ifndef COMMON_FORMAT_H
#define COMMON_FORMAT_H

#include "common/macros.h"

namespace sicxe {

// Contains an enum with all possible instruction formats.
class Format {
 public:
  DISALLOW_INSTANTIATE(Format);

  enum FormatId {
    F1 = 0,
    F2,
    FS34,
    // Should be the last element in this enum
    NUM_FORMATS
  };
};

// Assembler syntax for instructions of the same format may differ. This class
// contains an enum containing all possible instruction syntax types.
class Syntax {
 public:
  DISALLOW_INSTANTIATE(Syntax);

  enum SyntaxId {
    // format 1
    F1_NONE = 0,

    // format 2
    F2_REG_REG,
    F2_REG_N,
    F2_REG,

    // format 3
    FS34_LOAD_W,   // memory operand is a word
    FS34_LOAD_B,   // memory operand is a byte
    FS34_LOAD_F,   // memory operand is a float
    FS34_STORE_W,  // memory operand is a word, operand is modified
    FS34_STORE_B,  // memory operand is a byte, operand is modified
    FS34_STORE_F,  // memory operand is a float, operand is modified
    FS34_NONE,

    // Should be the last element in this enum
    NUM_SYNTAXES
  };
};

}  // namespace sicxe

#endif  // COMMON_FORMAT_H
