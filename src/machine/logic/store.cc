#include "machine/logic/store.h"

#include <assert.h>
#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// StoreWord implementation
StoreWord::StoreWord(CpuState::RegisterId implicit_register)
  : implicit_register_(implicit_register) {}
StoreWord::~StoreWord() {}

ExecuteResult::ResultId StoreWord::Execute(const InstructionInstance& instance,
                                                Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = machine->cpu_state().target_address;
  machine->WriteMemoryWord(address, machine->cpu_state().registers[implicit_register_]);
  return ExecuteResult::OK;
}

// StoreByte implementation
StoreByte::StoreByte() {}
StoreByte::~StoreByte() {}

ExecuteResult::ResultId StoreByte::Execute(const InstructionInstance& instance,
                                                Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = machine->cpu_state().target_address;
  machine->WriteMemoryByte(address, machine->cpu_state().registers[CpuState::REG_A]);
  return ExecuteResult::OK;
}

// StoreFlags implementation
StoreFlags::StoreFlags() {}
StoreFlags::~StoreFlags() {}

ExecuteResult::ResultId StoreFlags::Execute(const InstructionInstance& instance,
                                            Machine* machine) const {
  assert(instance.format == Format::FS34);
  unused(instance);

  uint8 cc = static_cast<uint8>(machine->cpu_state().condition_code);
  uint8 icc = static_cast<uint8>(machine->cpu_state().interrupt_condition_code);
  uint8 i = machine->cpu_state().interrupt_enabled ? 1 : 0;

  uint8 sw = (i << 4) || (icc << 2) || cc;
  uint32 address = machine->cpu_state().target_address;
  machine->WriteMemoryByte(address, sw);
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
