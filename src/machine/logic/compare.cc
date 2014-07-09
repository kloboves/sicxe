#include "machine/logic/compare.h"

#include <assert.h>
#include "common/format_util.h"
#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// CompareMem implementation
CompareMem::CompareMem() {}
CompareMem::~CompareMem() {}

ExecuteResult::ResultId CompareMem::Execute(const InstructionInstance& instance,
                                                 Machine* machine) const {
  assert(instance.format == Format::FS34);

  uint32 operand = 0;
  if (IsImmediateAddressing(instance)) {
    operand = machine->cpu_state().target_address;
  } else {
    operand = machine->ReadMemoryWord(machine->cpu_state().target_address);
  }

  int32 a = Machine::SignExtendWord(machine->cpu_state().registers[CpuState::REG_A]);
  int32 b = Machine::SignExtendWord(operand);

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

// CompareReg implementation
CompareReg::CompareReg() {}
CompareReg::~CompareReg() {}

ExecuteResult::ResultId CompareReg::Execute(const InstructionInstance& instance,
                                        Machine* machine) const {
  assert(instance.format == Format::F2);

  uint8 r1 = instance.operands.f2.r1;
  uint8 r2 = instance.operands.f2.r2;
  if (r1 >= CpuState::NUM_REGISTERS || r2 >= CpuState::NUM_REGISTERS) {
    return ExecuteResult::INVALID_ADDRESSING;
  }

  int32 a = Machine::SignExtendWord(machine->cpu_state().registers[r1]);
  int32 b = Machine::SignExtendWord(machine->cpu_state().registers[r2]);

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

// IncrementCompareMem implementation
IncrementCompareMem::IncrementCompareMem() {}
IncrementCompareMem::~IncrementCompareMem() {}

ExecuteResult::ResultId IncrementCompareMem::Execute(
    const InstructionInstance& instance, Machine* machine) const {
  assert(instance.format == Format::FS34);

  uint32 operand = 0;
  if (IsImmediateAddressing(instance)) {
    operand = machine->cpu_state().target_address;
  } else {
    operand = machine->ReadMemoryWord(machine->cpu_state().target_address);
  }
  int32 value = Machine::SignExtendWord(operand);

  int32 x = Machine::SignExtendWord(machine->cpu_state().registers[CpuState::REG_X]);
  machine->mutable_cpu_state()->registers[CpuState::REG_X] = Machine::TrimWord(x + 1);
  x = Machine::SignExtendWord(machine->cpu_state().registers[CpuState::REG_X]);

  CpuState::ConditionId condition_code = CpuState::EQUAL;
  if (x > value) {
    condition_code = CpuState::GREATER;
  }
  if (x < value) {
    condition_code = CpuState::LESS;
  }
  machine->mutable_cpu_state()->condition_code = condition_code;
  return ExecuteResult::OK;
}

// IncrementCompareReg implementation
IncrementCompareReg::IncrementCompareReg() {}
IncrementCompareReg::~IncrementCompareReg() {}

ExecuteResult::ResultId IncrementCompareReg::Execute(
    const InstructionInstance& instance, Machine* machine) const {
  assert(instance.format == Format::F2);

  uint8 r1 = instance.operands.f2.r1;
  if (r1 >= CpuState::NUM_REGISTERS) {
    return ExecuteResult::INVALID_ADDRESSING;
  }
  int32 value = Machine::SignExtendWord(machine->cpu_state().registers[r1]);

  int32 x = Machine::SignExtendWord(machine->cpu_state().registers[CpuState::REG_X]);
  machine->mutable_cpu_state()->registers[CpuState::REG_X] = Machine::TrimWord(x + 1);
  x = Machine::SignExtendWord(machine->cpu_state().registers[CpuState::REG_X]);

  CpuState::ConditionId condition_code = CpuState::EQUAL;
  if (x > value) {
    condition_code = CpuState::GREATER;
  }
  if (x < value) {
    condition_code = CpuState::LESS;
  }
  machine->mutable_cpu_state()->condition_code = condition_code;
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
