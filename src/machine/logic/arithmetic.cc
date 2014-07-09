#include "machine/logic/arithmetic.h"

#include <assert.h>
#include "common/format_util.h"
#include "common/instruction_instance.h"
#include "machine/machine.h"

namespace sicxe {
namespace machine {
namespace logic {

// ArithmeticMem implementation
ArithmeticMem::ArithmeticMem(ArithmeticOperation::OperationId operation)
  : operation_(operation) {}
ArithmeticMem::~ArithmeticMem() {}

ExecuteResult::ResultId ArithmeticMem::Execute(const InstructionInstance& instance,
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

  switch (operation_) {
    case ArithmeticOperation::ADD:
      a += b;
      break;
    case ArithmeticOperation::SUBTRACT:
      a -= b;
      break;
    case ArithmeticOperation::MULTIPLY:
      a *= b;
      break;
    case ArithmeticOperation::DIVIDE:
      a /= b;
      break;
    case ArithmeticOperation::AND:
      a &= b;
      break;
    case ArithmeticOperation::OR:
      a |= b;
      break;
    case ArithmeticOperation::XOR:
      a ^= b;
      break;
  }

  machine->mutable_cpu_state()->registers[CpuState::REG_A] = Machine::TrimWord(a);
  return ExecuteResult::OK;
}

// ArithmeticReg implementation
ArithmeticReg::ArithmeticReg(ArithmeticOperation::OperationId operation)
  : operation_(operation) {}
ArithmeticReg::~ArithmeticReg() {}

ExecuteResult::ResultId ArithmeticReg::Execute(const InstructionInstance& instance,
                                               Machine* machine) const {
  assert(instance.format == Format::F2);

  uint8 r1 = instance.operands.f2.r1;
  uint8 r2 = instance.operands.f2.r2;
  if (r1 >= CpuState::NUM_REGISTERS || r2 >= CpuState::NUM_REGISTERS) {
    return ExecuteResult::INVALID_ADDRESSING;
  }

  int32 a = Machine::SignExtendWord(machine->cpu_state().registers[r2]);
  int32 b = Machine::SignExtendWord(machine->cpu_state().registers[r1]);

  switch (operation_) {
    case ArithmeticOperation::ADD:
      a += b;
      break;
    case ArithmeticOperation::SUBTRACT:
      a -= b;
      break;
    case ArithmeticOperation::MULTIPLY:
      a *= b;
      break;
    case ArithmeticOperation::DIVIDE:
      a /= b;
      break;
    case ArithmeticOperation::AND:
      a &= b;
      break;
    case ArithmeticOperation::OR:
      a |= b;
      break;
    case ArithmeticOperation::XOR:
      a ^= b;
      break;
  }

  machine->mutable_cpu_state()->registers[r2] = Machine::TrimWord(a);
  return ExecuteResult::OK;
}

// RegisterNegate implementation
RegisterNegate::RegisterNegate() {}
RegisterNegate::~RegisterNegate() {}

ExecuteResult::ResultId RegisterNegate::Execute(const InstructionInstance& instance,
                                                Machine* machine) const {
  assert(instance.format == Format::F2);

  uint8 r1 = instance.operands.f2.r1;
  if (r1 >= CpuState::NUM_REGISTERS) {
    return ExecuteResult::INVALID_ADDRESSING;
  }

  int32 value = Machine::SignExtendWord(machine->cpu_state().registers[r1]);
  value = ~value;
  machine->mutable_cpu_state()->registers[r1] = Machine::TrimWord(value);
  return ExecuteResult::OK;
}

}  // namespace logic
}  // namespace machine
}  // namespace sicxe
