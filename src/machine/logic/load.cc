#include "machine/logic/load.h"

#include <assert.h>
#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// LoadWord implementation
LoadWord::LoadWord(CpuState::RegisterId implicit_register)
  : implicit_register_(implicit_register) {}
LoadWord::~LoadWord() {}

ExecuteResult::ResultId LoadWord::Execute(const InstructionInstance& instance,
                                               Machine* machine) const {
  assert(instance.format == Format::FS34);

  uint32 value = 0;
  if (IsImmediateAddressing(instance)) {
    value = machine->cpu_state().target_address;
  } else {
    value = machine->ReadMemoryWord(machine->cpu_state().target_address);
  }

  machine->mutable_cpu_state()->registers[implicit_register_] = value;
  return ExecuteResult::OK;
}

// LoadByte implementation
LoadByte::LoadByte() {}
LoadByte::~LoadByte() {}

ExecuteResult::ResultId LoadByte::Execute(const InstructionInstance& instance,
                                               Machine* machine) const {
  assert(instance.format == Format::FS34);

  uint32 value = 0;
  if (IsImmediateAddressing(instance)) {
    value = machine->cpu_state().target_address & 0xff;
  } else {
    value = machine->ReadMemoryByte(machine->cpu_state().target_address);
  }

  machine->mutable_cpu_state()->registers[CpuState::REG_A] = value;
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
