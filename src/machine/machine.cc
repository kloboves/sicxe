#include "machine/machine.h"

#include <string.h>
#include "common/format.h"
#include "common/format_util.h"
#include "common/instruction.h"
#include "common/instruction_db.h"
#include "common/instruction_instance.h"
#include "machine/device.h"
#include "machine/logic.h"
#include "machine/logic_db.h"

namespace sicxe {
namespace machine {

const size_t Machine::kMemorySize = 1 << 20;  // 1MB

int32 Machine::SignExtendWord(uint32 word) {
  uint32 result = 0xff000000;
  if (((word >> 23) & 0x1) == 0x0) {
    result = 0;
  }
  result |= word;
  return static_cast<int32>(result);
}

uint32 Machine::TrimWord(uint32 word) {
  return word & 0xffffff;
}

uint32 Machine::TrimAddress(uint32 address) {
  return address & 0xfffff;
}

Machine::Machine() : Machine(InstructionDB::Default(), LogicDB::Default()) {}

Machine::Machine(const InstructionDB* instruction_db, const LogicDB* logic_db)
  : instruction_db_(instruction_db), logic_db_(logic_db), cpu_state_(),
    memory_(new uint8[kMemorySize]) {
  Reset();
}

Machine::~Machine() {}

void Machine::Reset() {
  cpu_state_ = CpuState();
  memset(memory_.get(), 0x00, kMemorySize);
}

namespace {

uint32 SignExtendF3Operand(uint32 address) {
  uint32 result = 0xfffff000;
  if (((address >> 11) & 0x1) == 0x0) {
    result = 0;
  }
  result |= address;
  return result;
}

}  // namespace

bool Machine::CalculateTargetAddress(const InstructionInstance& instance) {
  const auto& operands = instance.operands.fS34;
  bool indirect = false;
  uint32 target_address = 0;
  if (!operands.n && !operands.i) {  // SIC format
    target_address = operands.address;
    if (operands.x) {
      target_address += cpu_state_.registers[CpuState::REG_X];
    }
  } else {
    if (!operands.n && operands.i) {  // immediate
      if (operands.x) {  // indexed not allowed
        return false;
      }
    } else if (operands.n && !operands.i) {  // indirect
      if (operands.x) {  // indexed not allowed
        return false;
      }
      indirect = true;
    }

    // PC-relative and base addressing not allowed in format 4
    if (operands.e && (operands.p || operands.b)) {
      return false;
    }
    // PC-relative and base addressing not allowed together
    if (operands.p && operands.b) {
      return false;
    }

    if (operands.p) {  // sign extend
      target_address = SignExtendF3Operand(operands.address);
    } else {
      target_address = operands.address;
    }

    if (operands.p) {
      target_address += cpu_state_.program_counter;
    }
    if (operands.b) {
      target_address += cpu_state_.registers[CpuState::REG_B];
    }
    if (operands.x) {
      target_address += cpu_state_.registers[CpuState::REG_X];
    }
  }

  target_address = TrimWord(target_address);
  if (indirect) {
    target_address = TrimAddress(target_address);
    target_address = ReadMemoryWord(target_address);
  }
  cpu_state_.target_address = target_address;
  return true;
}

ExecuteResult::ResultId Machine::Execute() {
  uint32 program_counter = cpu_state_.program_counter;

  // delayed interrupt enable
  if (cpu_state_.interrupt_enable_next) {
    cpu_state_.interrupt_enabled = true;
    cpu_state_.interrupt_enable_next = false;
  }

  // read instruction from memory
  uint8 instruction_buffer[4];
  ReadMemory(program_counter, 4, instruction_buffer);

  // decode instruction
  InstructionInstance instance;
  int instruction_length = 0;
  if (!FormatUtil::Decode(instruction_db_, instruction_buffer,
                          &instance, &instruction_length)) {
    return ExecuteResult::INVALID_OPCODE;
  }

  // find instruction logic
  const InstructionLogic* logic = logic_db_->Find(instance.opcode);
  if (logic == nullptr) {
    return ExecuteResult::NOT_IMPLEMENTED;
  }

  // advance program counter
  uint32 new_program_counter = TrimAddress(program_counter + instruction_length);
  cpu_state_.program_counter = new_program_counter;

  // calculate target address for FS34 instructions
  if (instance.format == Format::FS34 && !CalculateTargetAddress(instance)) {
    cpu_state_.program_counter = program_counter;  // restore previous program counter
    return ExecuteResult::INVALID_ADDRESSING;
  }

  // execute instruction
  ExecuteResult::ResultId result = logic->Execute(instance, this);
  if (result != ExecuteResult::OK) {
    cpu_state_.program_counter = program_counter;  // restore previous program counter
  }
  return result;
}

void Machine::Interrupt() {
  if (!cpu_state_.interrupt_enabled) {
    return;
  }
  cpu_state_.interrupt_enabled = false;
  cpu_state_.interrupt_enable_next = false;
  cpu_state_.interrupt_link = cpu_state_.program_counter;
  cpu_state_.interrupt_condition_code = cpu_state_.condition_code;
  cpu_state_.program_counter = TrimAddress(ReadMemoryWord(0xffffd));
}

void Machine::ReadMemory(uint32 address, int read_size, uint8* buffer) const {
  for (int i = 0; i < read_size; i++, address++) {
    address = TrimAddress(address);
    buffer[i] = memory_[address];
  }
}

uint8 Machine::ReadMemoryByte(uint32 address) const {
  address = TrimAddress(address);
  return memory_[address];
}

uint32 Machine::ReadMemoryWord(uint32 address) const {
  uint32 result = 0;
  address = TrimAddress(address);
  result = static_cast<uint32>(memory_[address]);
  result <<= 8;
  address = TrimAddress(address + 1);
  result |= static_cast<uint32>(memory_[address]);
  result <<= 8;
  address = TrimAddress(address + 1);
  result |= static_cast<uint32>(memory_[address]);
  return result;
}

void Machine::ReadMemoryFloat(uint32 address, uint8* result) const {
  for (int i = 0; i < 6; i++, address++) {
    address = TrimAddress(address);
    result[i] = memory_[address];
  }
}

void Machine::WriteMemory(uint32 address, int write_size, const uint8* buffer) {
  for (int i = 0; i < write_size; i++, address++) {
    address = TrimAddress(address);
    memory_[address] = buffer[i];
  }
}

void Machine::WriteMemoryByte(uint32 address, uint8 value) {
  address = TrimAddress(address);
  memory_[address] = value;
}

void Machine::WriteMemoryWord(uint32 address, uint32 value) {
  uint8 byte = 0;
  address = TrimAddress(address);
  byte = (value >> 16) & 0xff;
  memory_[address] = byte;
  address = TrimAddress(address + 1);
  byte = (value >> 8) & 0xff;
  memory_[address] = byte;
  address = TrimAddress(address + 1);
  byte = value & 0xff;
  memory_[address] = byte;
}

void Machine::WriteMemoryFloat(uint32 address, const uint8* value) {
  for (int i = 0; i < 6; i++, address++) {
    address = TrimAddress(address);
    memory_[address] = value[i];
  }
}

Device* Machine::GetDevice(uint8 device_id) {
  return devices_[device_id].get();
}

void Machine::SetDevice(uint8 device_id, Device* device) {
  devices_[device_id].reset(device);
}

Device* Machine::ReleaseDevice(uint8 device_id) {
  return devices_[device_id].release();
}

const CpuState& Machine::cpu_state() const {
  return cpu_state_;
}

CpuState* Machine::mutable_cpu_state() {
  return &cpu_state_;
}

const uint8* Machine::memory() const {
  return memory_.get();
}

}  // namespace machine
}  // namespace sicxe
