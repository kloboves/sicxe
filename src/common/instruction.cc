#include "common/instruction.h"

namespace sicxe {

Instruction::Instruction(uint8 the_opcode, const std::string& the_mnemonic,
                         Format::FormatId the_format, Syntax::SyntaxId the_syntax)
    : opcode_(the_opcode), mnemonic_(the_mnemonic), format_(the_format),
      syntax_(the_syntax) {}

Instruction::~Instruction() {}

uint8 Instruction::opcode() const {
  return opcode_;
}

const std::string& Instruction::mnemonic() const {
  return mnemonic_;
}

Format::FormatId Instruction::format() const {
  return format_;
}

Syntax::SyntaxId Instruction::syntax() const {
  return syntax_;
}

}  // namespace sicxe
