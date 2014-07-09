#include "machine/logic/move.h"

#include <assert.h>
#include "common/cpu_state.h"
#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// ClearReg implementation
ClearReg::ClearReg() {}
ClearReg::~ClearReg() {}

ExecuteResult::ResultId ClearReg::Execute(const InstructionInstance& instance,
                                          Machine* machine) const {
  assert(instance.format == Format::F2);

  uint8 r1 = instance.operands.f2.r1;
  if (r1 >= CpuState::NUM_REGISTERS) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  machine->mutable_cpu_state()->registers[r1] = 0;
  return ExecuteResult::OK;
}

// MoveReg implementation
MoveReg::MoveReg() {}
MoveReg::~MoveReg() {}

ExecuteResult::ResultId MoveReg::Execute(const InstructionInstance& instance,
                                         Machine* machine) const {
  assert(instance.format == Format::F2);

  uint8 r1 = instance.operands.f2.r1;
  uint8 r2 = instance.operands.f2.r2;
  if (r1 >= CpuState::NUM_REGISTERS || r2 >= CpuState::NUM_REGISTERS) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  machine->mutable_cpu_state()->registers[r2] = machine->cpu_state().registers[r1];
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
