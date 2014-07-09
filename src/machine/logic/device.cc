#include "machine/logic/device.h"

#include <assert.h>
#include "common/instruction_instance.h"
#include "machine/device.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// TestDevice implementation
TestDevice::TestDevice() {}
TestDevice::~TestDevice() {}

ExecuteResult::ResultId TestDevice::Execute(const InstructionInstance& instance,
                                            Machine* machine) const {
  assert(instance.format == Format::FS34);

  uint8 device_id = 0;
  if (IsImmediateAddressing(instance)) {
    device_id = machine->cpu_state().target_address & 0xff;
  } else {
    device_id = machine->ReadMemoryByte(machine->cpu_state().target_address);
  }

  Device* device = machine->GetDevice(device_id);
  if (device == nullptr || !device->Test()) {
    machine->mutable_cpu_state()->condition_code = CpuState::GREATER;
  } else {
    machine->mutable_cpu_state()->condition_code = CpuState::LESS;
  }
  return ExecuteResult::OK;
}

// ReadDevice implementation
ReadDevice::ReadDevice() {}
ReadDevice::~ReadDevice() {}

ExecuteResult::ResultId ReadDevice::Execute(const InstructionInstance& instance,
                                            Machine* machine) const {
  assert(instance.format == Format::FS34);

  uint8 device_id = 0;
  if (IsImmediateAddressing(instance)) {
    device_id = machine->cpu_state().target_address & 0xff;
  } else {
    device_id = machine->ReadMemoryByte(machine->cpu_state().target_address);
  }

  Device* device = machine->GetDevice(device_id);
  uint8 value = 0;
  if (device == nullptr || !device->Read(&value)) {
    return ExecuteResult::DEVICE_ERROR;
  }
  machine->mutable_cpu_state()->registers[CpuState::REG_A] = value;
  return ExecuteResult::OK;
}

// WriteDevice implementation
WriteDevice::WriteDevice() {}
WriteDevice::~WriteDevice() {}

ExecuteResult::ResultId WriteDevice::Execute(const InstructionInstance& instance,
                                             Machine* machine) const {
  assert(instance.format == Format::FS34);

  uint8 device_id = 0;
  if (IsImmediateAddressing(instance)) {
    device_id = machine->cpu_state().target_address & 0xff;
  } else {
    device_id = machine->ReadMemoryByte(machine->cpu_state().target_address);
  }

  Device* device = machine->GetDevice(device_id);
  uint8 value = machine->cpu_state().registers[CpuState::REG_A] & 0xff;
  if (device == nullptr || !device->Write(value)) {
    return ExecuteResult::DEVICE_ERROR;
  }
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
