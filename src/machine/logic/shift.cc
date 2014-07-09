#include "machine/logic/shift.h"

#include <assert.h>
#include "common/cpu_state.h"
#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

Shift::Shift(bool direction_right) : direction_right_(direction_right) {}
Shift::~Shift() {}

ExecuteResult::ResultId Shift::Execute(const InstructionInstance& instance,
                                       Machine* machine) const {
  assert(instance.format == Format::F2);

  uint8 r = instance.operands.f2.r1;
  uint8 n = instance.operands.f2.r2;
  if (r >= CpuState::NUM_REGISTERS) {
    return ExecuteResult::INVALID_ADDRESSING;
  }

  int32 value = Machine::SignExtendWord(machine->cpu_state().registers[r]);
  if (direction_right_) {
    value >>= n;
  } else {
    value <<= n;
  }
  machine->mutable_cpu_state()->registers[r] = Machine::TrimWord(value);
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
