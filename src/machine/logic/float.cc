#include "machine/logic/float.h"

#include <assert.h>
#include "common/float_util.h"
#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// FloatToInt implementation
FloatToInt::FloatToInt() {}
FloatToInt::~FloatToInt() {}

ExecuteResult::ResultId FloatToInt::Execute(const InstructionInstance& instance,
                                            Machine* machine) const {
  assert(instance.format == Format::F1);
  unused(instance);

  double value_double = FloatUtil::DecodeFloatData(machine->cpu_state().float_register);
  int32 value = static_cast<int32>(value_double);
  machine->mutable_cpu_state()->registers[CpuState::REG_A] = Machine::TrimWord(value);
  return ExecuteResult::OK;
}

// IntToFloat implementation
IntToFloat::IntToFloat() {}
IntToFloat::~IntToFloat() {}

ExecuteResult::ResultId IntToFloat::Execute(const InstructionInstance& instance,
                                            Machine* machine) const {
  assert(instance.format == Format::F1);
  unused(instance);

  int32 value = Machine::SignExtendWord(machine->cpu_state().registers[CpuState::REG_A]);
  double value_double = static_cast<double>(value);
  FloatUtil::EncodeFloatData(value_double, machine->mutable_cpu_state()->float_register);
  return ExecuteResult::OK;
}

// FloatArithmetic implementation
FloatArithmetic::FloatArithmetic(OperationId operation) : operation_(operation) {}
FloatArithmetic::~FloatArithmetic() {}

ExecuteResult::ResultId FloatArithmetic::Execute(const InstructionInstance& instance,
                                                 Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = machine->cpu_state().target_address;
  uint8 float_data[6];
  machine->ReadMemoryFloat(address, float_data);

  double a = FloatUtil::DecodeFloatData(machine->cpu_state().float_register);
  double b = FloatUtil::DecodeFloatData(float_data);
  switch (operation_) {
    case ADD:
      a += b;
      break;
    case SUBTRACT:
      a -= b;
      break;
    case MULTIPLY:
      a *= b;
      break;
    case DIVIDE:
      a /= b;
      break;
  }
  FloatUtil::EncodeFloatData(a, machine->mutable_cpu_state()->float_register);
  return ExecuteResult::OK;
}

// FloatCompare implementation
FloatCompare::FloatCompare() {}
FloatCompare::~FloatCompare() {}

ExecuteResult::ResultId FloatCompare::Execute(const InstructionInstance& instance,
                                              Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = machine->cpu_state().target_address;
  uint8 float_data[6];
  machine->ReadMemoryFloat(address, float_data);

  double a = FloatUtil::DecodeFloatData(machine->cpu_state().float_register);
  double b = FloatUtil::DecodeFloatData(float_data);

  CpuState::ConditionId condition_code = CpuState::EQUAL;
  if (a > b) {
    condition_code = CpuState::GREATER;
  }
  if (a < b) {
    condition_code = CpuState::LESS;
  }
  machine->mutable_cpu_state()->condition_code = condition_code;
  return ExecuteResult::OK;
}

// LoadFloat implementation
LoadFloat::LoadFloat() {}
LoadFloat::~LoadFloat() {}

ExecuteResult::ResultId LoadFloat::Execute(const InstructionInstance& instance,
                                           Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = machine->cpu_state().target_address;
  machine->ReadMemoryFloat(address, machine->mutable_cpu_state()->float_register);
  return ExecuteResult::OK;
}

// StoreFloat implementation
StoreFloat::StoreFloat() {}
StoreFloat::~StoreFloat() {}

ExecuteResult::ResultId StoreFloat::Execute(const InstructionInstance& instance,
                                            Machine* machine) const {
  assert(instance.format == Format::FS34);

  if (IsImmediateAddressing(instance)) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  uint32 address = machine->cpu_state().target_address;
  machine->WriteMemoryFloat(address, machine->cpu_state().float_register);
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
