#include "machine/logic/interrupt.h"

#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// InterruptEnable implementation
InterruptEnable::InterruptEnable() {}
InterruptEnable::~InterruptEnable() {}

ExecuteResult::ResultId InterruptEnable::Execute(const InstructionInstance& instance,
                                                 Machine* machine) const {
  assert(instance.format == Format::F1);
  unused(instance);

  machine->mutable_cpu_state()->interrupt_enable_next = true;
  return ExecuteResult::OK;
}

// InterruptDisable implementation
InterruptDisable::InterruptDisable() {}
InterruptDisable::~InterruptDisable() {}

ExecuteResult::ResultId InterruptDisable::Execute(const InstructionInstance& instance,
                                                  Machine* machine) const {
  assert(instance.format == Format::F1);
  unused(instance);

  machine->mutable_cpu_state()->interrupt_enabled = false;
  machine->mutable_cpu_state()->interrupt_enable_next = false;
  return ExecuteResult::OK;
}

// InterruptReturn implementation
InterruptReturn::InterruptReturn() {}
InterruptReturn::~InterruptReturn() {}

ExecuteResult::ResultId InterruptReturn::Execute(const InstructionInstance& instance,
                                                 Machine* machine) const {
  assert(instance.format == Format::F1);
  unused(instance);

  machine->mutable_cpu_state()->condition_code =
      machine->cpu_state().interrupt_condition_code;
  machine->mutable_cpu_state()->program_counter = machine->cpu_state().interrupt_link;
  return ExecuteResult::OK;
}

// InterruptLinkStore implementation
InterruptLinkStore::InterruptLinkStore() {}
InterruptLinkStore::~InterruptLinkStore() {}

ExecuteResult::ResultId InterruptLinkStore::Execute(const InstructionInstance& instance,
                                                    Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = machine->cpu_state().target_address;
  machine->WriteMemoryWord(address, machine->cpu_state().interrupt_link);
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
