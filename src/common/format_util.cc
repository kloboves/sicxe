#include "common/format_util.h"

#include "common/format.h"
#include "common/instruction.h"
#include "common/instruction_db.h"
#include "common/instruction_instance.h"

namespace sicxe {

bool FormatUtil::Decode(const InstructionDB* instruction_db,
                        const uint8* raw_insn_data,
                        InstructionInstance* instance,
                        int* decoded_length) {
  // decode opcode and find instruction
  const Instruction* instruction = nullptr;
  {
    uint8 opcode_short = raw_insn_data[0] & 0xFC;
    const Instruction* instruction_short = instruction_db->FindOpcode(opcode_short);
    if (instruction_short != nullptr &&
        instruction_short->format() == Format::FS34) {
      instance->opcode = opcode_short;
      instruction = instruction_short;
    } else {
      uint8 opcode_long = raw_insn_data[0];
      const Instruction* instruction_long = instruction_db->FindOpcode(opcode_long);
      if (instruction_long != nullptr) {
        instance->opcode = opcode_long;
        instruction = instruction_long;
      }
    }
  }
  if (instruction == nullptr) {
    // instruction does not exist
    return false;
  }
  instance->format = instruction->format();

  // decode operands
  if (instance->format == Format::F1) {
    *decoded_length = 1;
  } else if (instance->format == Format::F2) {
    instance->operands.f2.r1 = raw_insn_data[1] >> 4;
    instance->operands.f2.r2 = raw_insn_data[1] & 0x0F;
    *decoded_length = 2;
  } else if (instance->format == Format::FS34) {
    instance->operands.fS34.n = (((raw_insn_data[0] >> 1) & 0x1) == 1);
    instance->operands.fS34.i = ((raw_insn_data[0] & 0x1) == 1);
    instance->operands.fS34.x = (((raw_insn_data[1] >> 7) & 0x1) == 1);
    if (instance->operands.fS34.n == false &&
        instance->operands.fS34.i == false) { //  format S
      instance->operands.fS34.b = false;
      instance->operands.fS34.p = false;
      instance->operands.fS34.e = false;
      instance->operands.fS34.address = raw_insn_data[1] & 0x7F;
      instance->operands.fS34.address <<= 8;
      instance->operands.fS34.address |= raw_insn_data[2];
      *decoded_length = 3;
    } else {
      instance->operands.fS34.b = (((raw_insn_data[1] >> 6) & 0x1) == 1);
      instance->operands.fS34.p = (((raw_insn_data[1] >> 5) & 0x1) == 1);
      instance->operands.fS34.e = (((raw_insn_data[1] >> 4) & 0x1) == 1);
      instance->operands.fS34.address = raw_insn_data[1] & 0xF;
      instance->operands.fS34.address <<= 8;
      instance->operands.fS34.address |= raw_insn_data[2];
      if (instance->operands.fS34.e) {  // format 4
        instance->operands.fS34.address <<= 8;
        instance->operands.fS34.address |= raw_insn_data[3];
        *decoded_length = 4;
      } else {
        *decoded_length = 3;
      }
    }
  }
  return true;
}

namespace {

void SetBit(bool value, int bit, uint8* data) {
  uint8 v = value ? 0x1 : 0x0;
  v <<= bit;
  *data |= v;
}

}  // namespace

bool FormatUtil::Encode(const InstructionInstance& instance,
                        uint8* raw_insn_data,
                        int* encoded_length) {
  raw_insn_data[0] = instance.opcode;
  if (instance.format == Format::F1) {
    *encoded_length = 1;
  } else if (instance.format == Format::F2) {
    raw_insn_data[1] = instance.operands.f2.r1 << 4;
    raw_insn_data[1] |= instance.operands.f2.r2 & 0xF;
    *encoded_length = 2;
  } else if (instance.format == Format::FS34) {
    if ((instance.opcode & 0x3) != 0x0) {
      // last 2 bits of opcode should be 0
      return false;
    }
    SetBit(instance.operands.fS34.n, 1, &raw_insn_data[0]);
    SetBit(instance.operands.fS34.i, 0, &raw_insn_data[0]);
    raw_insn_data[1] = 0x0;
    SetBit(instance.operands.fS34.x, 7, &raw_insn_data[1]);
    uint32 address = instance.operands.fS34.address;
    if (instance.operands.fS34.n == false &&
        instance.operands.fS34.i == false) {  // format S
      raw_insn_data[2] = address & 0xFF;
      address >>= 8;
      raw_insn_data[1] |= address & 0x7F;
      *encoded_length = 3;
    } else {
      SetBit(instance.operands.fS34.b, 6, &raw_insn_data[1]);
      SetBit(instance.operands.fS34.p, 5, &raw_insn_data[1]);
      SetBit(instance.operands.fS34.e, 4, &raw_insn_data[1]);
      if (instance.operands.fS34.e) {  // format 4
        raw_insn_data[3] = address & 0xFF;
        address >>= 8;
      }
      raw_insn_data[2] = address & 0xFF;
      address >>= 8;
      raw_insn_data[1] |= address & 0xF;
      if (instance.operands.fS34.e) {  // format 4
        *encoded_length = 4;
      } else {  // format 3
        *encoded_length = 3;
      }
    }
  } else {
    return false;
  }
  return true;
}

}  // namespace sicxe
