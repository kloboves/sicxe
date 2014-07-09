#ifndef COMMON_INSTRUCTION_H
#define COMMON_INSTRUCTION_H

#include <string>
#include "common/format.h"
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class Instruction {
 public:
  DISALLOW_COPY_AND_MOVE(Instruction);

  Instruction(uint8 the_opcode, const std::string& the_mnemonic,
              Format::FormatId the_format, Syntax::SyntaxId the_syntax);
  ~Instruction();

  uint8 opcode() const;
  const std::string& mnemonic() const;
  Format::FormatId format() const;
  Syntax::SyntaxId syntax() const;

 private:
  uint8 opcode_;
  std::string mnemonic_;
  Format::FormatId format_;
  Syntax::SyntaxId syntax_;
};

}  // namespace sicxe

#endif  // COMMON_INSTRUCTION_H
