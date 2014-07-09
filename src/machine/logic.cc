#include "machine/logic.h"

#include "common/instruction_instance.h"

namespace sicxe {
namespace machine {

bool InstructionLogic::IsImmediateAddressing(const InstructionInstance& instance) {
  return instance.format == Format::FS34 &&
         !instance.operands.fS34.n && instance.operands.fS34.i;
}

InstructionLogic::InstructionLogic() {}
InstructionLogic::~InstructionLogic() {}

}  // namespace machine
}  // namespace sicxe
