#include "machine/logic/jump.h"

#include <assert.h>
#include "common/format_util.h"
#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// Jump implementation
Jump::Jump(bool save_program_counter) : save_program_counter_(save_program_counter) {}
Jump::~Jump() {}

ExecuteResult::ResultId Jump::Execute(const InstructionInstance& instance,
                                      Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = Machine::TrimAddress(machine->cpu_state().target_address);

  // detect endless loop
  uint32 instruction_length = instance.operands.fS34.e ? 4 : 3;
  if (Machine::TrimAddress(address + instruction_length) ==
      machine->cpu_state().program_counter) {
    return ExecuteResult::ENDLESS_LOOP;
  }

  if (save_program_counter_) {
    machine->mutable_cpu_state()->registers[CpuState::REG_L] =
      machine->cpu_state().program_counter;
  }
  machine->mutable_cpu_state()->program_counter = address;
  return ExecuteResult::OK;
}

// JumpConditional implementation
JumpConditional::JumpConditional(CpuState::ConditionId condition)
    : condition_(condition) {}
JumpConditional::~JumpConditional() {}

ExecuteResult::ResultId JumpConditional::Execute(const InstructionInstance& instance,
                                                 Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = Machine::TrimAddress(machine->cpu_state().target_address);
  if (machine->cpu_state().condition_code == condition_) {
    machine->mutable_cpu_state()->program_counter = address;
  }
  return ExecuteResult::OK;
}

// Return implementation
Return::Return() {}
Return::~Return() {}

ExecuteResult::ResultId Return::Execute(const InstructionInstance& instance,
                                        Machine* machine) const {
  assert(instance.format == Format::FS34);
  unused(instance);
  machine->mutable_cpu_state()->program_counter =
    Machine::TrimAddress(machine->cpu_state().registers[CpuState::REG_L]);
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
