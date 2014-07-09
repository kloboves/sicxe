#include "simulator/simulator.h"

#include <assert.h>
#include <string.h>
#include <map>
#include <string>
#include "common/format.h"
#include "common/format_util.h"
#include "common/instruction.h"
#include "common/instruction_db.h"
#include "common/instruction_instance.h"
#include "common/macros.h"

using std::string;

namespace sicxe {
namespace simulator {

const int Simulator::kDisassembleDefaultCount = 10;
const int Simulator::kDisassembleMaxCount = 70;

namespace {

void PrintRegisterName(uint8 reg) {
  if (reg < CpuState::NUM_REGISTERS) {
    string reg_name;
    bool success = CpuState::RegisterIdToName(
        static_cast<CpuState::RegisterId>(reg), &reg_name);
    assert(success);
    unused(success);
    printf("%s", reg_name.c_str());
  } else {
    printf("?");
  }
}

int SignExtendF3Operand(uint32 address) {
  uint32 result = 0xfffff000;
  if (((address >> 11) & 0x1) == 0x0) {
    result = 0;
  }
  result |= address;
  return static_cast<int>(result);
}

}  // namespace

int Simulator::DisassembleInstruction(uint32 address) {
  // read and decode instruction
  InstructionInstance instance;
  uint8 instruction_buffer[4];
  int instruction_length = 0;
  machine_.ReadMemory(address, 4, instruction_buffer);
  if (!FormatUtil::Decode(instruction_db_, instruction_buffer, &instance,
                          &instruction_length)) {
    return 0;
  }
  const Instruction* instruction = instruction_db_->FindOpcode(instance.opcode);

  printf(" %06x ", address);
  for (int i = 0; i < 4; i++) {
    printf(" ");
    if (i < instruction_length) {
      printf("%02x", static_cast<uint32>(instruction_buffer[i]));
    } else {
      printf("  ");
    }
  }
  printf("  ");

  if (instruction->syntax() == Syntax::F1_NONE ||
      instruction->syntax() == Syntax::FS34_NONE) {
    printf("%s", instruction->mnemonic().c_str());
  } else if (instruction->syntax() == Syntax::F2_REG_REG) {
    printf("%-12s", instruction->mnemonic().c_str());
    PrintRegisterName(instance.operands.f2.r1);
    printf(", ");
    PrintRegisterName(instance.operands.f2.r2);
  } else if (instruction->syntax() == Syntax::F2_REG_N) {
    printf("%-12s", instruction->mnemonic().c_str());
    PrintRegisterName(instance.operands.f2.r1);
    printf(", ");
    printf("%d", static_cast<int>(instance.operands.f2.r2));
  } else if (instruction->syntax() == Syntax::F2_REG) {
    printf("%-12s", instruction->mnemonic().c_str());
    PrintRegisterName(instance.operands.f2.r1);
  } else if (instruction->syntax() == Syntax::FS34_LOAD_W ||
             instruction->syntax() == Syntax::FS34_LOAD_B ||
             instruction->syntax() == Syntax::FS34_LOAD_F ||
             instruction->syntax() == Syntax::FS34_STORE_W ||
             instruction->syntax() == Syntax::FS34_STORE_B ||
             instruction->syntax() == Syntax::FS34_STORE_F) {
    const InstructionInstance::OperandsFS34& operands = instance.operands.fS34;
    bool invalid = false;
    bool immediate = false;
    bool simple = false;
    bool indirect = false;
    int value = 0;
    if (!operands.n && !operands.i) {
      simple = true;
      value = operands.address;
    } else {
      immediate = !operands.n && operands.i;
      simple = operands.n && operands.i;
      indirect = operands.n && !operands.i;

      if ((immediate || indirect) && operands.x) {
        invalid = true;
      }
      if (operands.e && (operands.p || operands.b)) {
        invalid = true;
      }
      if (operands.p && operands.b) {
        invalid = true;
      }

      if (operands.p) {
        value = SignExtendF3Operand(operands.address);
      } else {
        value = operands.address;
      }
    }

    if (!invalid && operands.e) {
      printf("+%-11s", instruction->mnemonic().c_str());
    } else {
      printf("%-12s", instruction->mnemonic().c_str());
    }

    if (invalid) {
      printf("invalid");
    } else {
      if (immediate) {
        printf("#");
      }
      if (indirect) {
        printf("@");
      }
      bool parentheses = !simple && (operands.b || operands.p || operands.x);
      if (parentheses) {
        printf("[");
      }
      printf("%d", value);
      if (operands.b) {
        printf(" + B");
      }
      if (operands.p) {
        printf(" + PC");
      }
      if (operands.x) {
        printf(" + X");
      }
      if (parentheses) {
        printf("]");
      }
    }
  }
  printf("\n");
  return instruction_length;
}

void Simulator::CommandDisassembleAutoOn(const CommandInterface::ParsedArgumentMap&) {
  auto_disassemble_ = true;
}

void Simulator::CommandDisassembleAutoOff(const CommandInterface::ParsedArgumentMap&) {
  auto_disassemble_ = false;
}

void Simulator::CommandDisassemble(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  int count = kDisassembleDefaultCount;
  {
    auto it = arguments.find("count");
    if (it != arguments.end()) {
      const CommandInterface::ParsedArgument& count_arg = it->second;
      if (!count_arg.is_word) {
        printf("Error: Count must be a number!\n");
        return;
      }
      count = count_arg.value_word;
    }
  }
  if (count > kDisassembleMaxCount) {
    printf("Error: Count must be less than or equal to %d!\n", kDisassembleMaxCount);
    return;
  }
  uint32 insn_address = address;
  for (int i = 0; i < count; i++) {
    int length = DisassembleInstruction(insn_address);
    if (length == 0) {
      break;
    }
    insn_address += length;
  }
}

}  // namespace simulator
}  // namespace sicxe
