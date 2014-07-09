#include "simulator/simulator.h"

#include <assert.h>
#include <stdio.h>
#include <memory>
#include <string>
#include "common/float_util.h"
#include "common/macros.h"

using sicxe::machine::Machine;
using std::string;

namespace sicxe {
namespace simulator {

namespace {

uint32 ConditionCodeToHex(CpuState::ConditionId condition_code) {
  switch (condition_code) {
    case CpuState::LESS:
      return 0;
    case CpuState::EQUAL:
      return 1;
    case CpuState::GREATER:
      return 2;
    default:
      return 0;
  }
}

string ConditionCodeToString(CpuState::ConditionId condition_code) {
  switch (condition_code) {
    case CpuState::LESS:
      return "LT";
    case CpuState::EQUAL:
      return "EQ";
    case CpuState::GREATER:
      return "GT";
    default:
      return "??";
  }
}

}  // namespace

void Simulator::CommandCpuPrint(const CommandInterface::ParsedArgumentMap&) {
  printf(" %-12s %-12s %-12s %-12s %-12s\n",
         "REGISTER", "HEX", "UNSIGNED", "SIGNED", "SPECIAL");
  uint32 pc = machine_.cpu_state().program_counter;
  string reg_name;
  printf(" %-12s %06x%-6s %u\n", "PC", pc, "", pc);
  for (int i = 0; i < CpuState::NUM_REGISTERS; i++) {
    CpuState::RegisterId reg_id = static_cast<CpuState::RegisterId>(i);
    bool success = CpuState::RegisterIdToName(reg_id, &reg_name);
    assert(success);
    unused(success);
    uint32 reg = machine_.cpu_state().registers[i];
    uint32 reg_signed = Machine::SignExtendWord(reg);
    printf(" %-12s %06x%-6s %-12u %-12d\n", reg_name.c_str(), reg, "", reg, reg_signed);
  }
  printf(" %-12s ", "F");
  const uint8* float_register = machine_.cpu_state().float_register;
  for (int i = 0; i < 6; i++) {
    printf("%02x", static_cast<uint32>(float_register[i]) & 0xff);
  }
  printf(" %-12s %-12s %lg\n", "", "", FloatUtil::DecodeFloatData(float_register));
  printf(" %-12s %01x%-11s %-12s %-12s %s\n", "CC",
         ConditionCodeToHex(machine_.cpu_state().condition_code), "", "", "",
         ConditionCodeToString(machine_.cpu_state().condition_code).c_str());
  printf(" %-12s %01x%-11s %-12s %-12s %s\n", "I",
         machine_.cpu_state().interrupt_enabled ? 1 : 0, "", "", "",
         machine_.cpu_state().interrupt_enabled ? "E" : "D");
  printf(" %-12s %06x%-6s %u\n", "IL", machine_.cpu_state().interrupt_link, "",
         machine_.cpu_state().interrupt_link);
  printf(" %-12s %01x%-11s %-12s %-12s %s\n", "ICC",
         ConditionCodeToHex(machine_.cpu_state().interrupt_condition_code), "", "", "",
         ConditionCodeToString(machine_.cpu_state().interrupt_condition_code).c_str());
}

namespace {

bool ArgumentToConditionCode(const CommandInterface::ParsedArgument& arg,
                             CpuState::ConditionId *condition_code) {
  if (arg.is_word) {
    switch (arg.value_word) {
      case 0:
        *condition_code = CpuState::LESS;
        break;
      case 1:
        *condition_code = CpuState::EQUAL;
        break;
      case 2:
        *condition_code = CpuState::GREATER;
        break;
      default:
        return false;
    }
    return true;
  }
  const string& val = arg.value_str;
  if (val == "LT") {
    *condition_code = CpuState::LESS;
  } else if (val == "EQ") {
    *condition_code = CpuState::EQUAL;
  } else if (val == "GT") {
    *condition_code = CpuState::GREATER;
  } else {
    return false;
  }
  return true;
}

}  // namespace

void Simulator::CommandCpuSet(const CommandInterface::ParsedArgumentMap& arguments) {
  const string& reg_name =
      ConvertToUppercase(arguments.find("register")->second.value_str);
  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (reg_name == "F") {
    const char* str_ptr = value_arg.value_str.c_str();
    char* end_ptr = nullptr;
    double value = strtod(str_ptr, &end_ptr);
    if (end_ptr != str_ptr + value_arg.value_str.size()) {
      printf("Error: Value is not a valid floating point number!\n");
      return;
    }
    FloatUtil::EncodeFloatData(value, machine_.mutable_cpu_state()->float_register);
  } else if (reg_name == "CC") {
    if (!ArgumentToConditionCode(
            value_arg, &machine_.mutable_cpu_state()->condition_code)) {
      printf("Error: Value must be LT, EQ or GT!\n");
      return;
    }
  } else if (reg_name == "ICC") {
    if (!ArgumentToConditionCode(
            value_arg, &machine_.mutable_cpu_state()->interrupt_condition_code)) {
      printf("Error: Value must be LT, EQ or GT!\n");
      return;
    }
  } else if (reg_name == "I") {
    bool value = false;
    bool success = true;
    if (value_arg.is_word) {
      switch (value_arg.value_word) {
        case 0:
          value = false;
          break;
        case 1:
          value = true;
          break;
        default:
          success = false;
          break;
      }
    } else {
      const string& val = value_arg.value_str;
      if (val == "D") {
        value = false;
      } else if (val == "E") {
        value = true;
      } else {
        success = false;
      }
    }
    if (!success) {
      printf("Error: Value must be E or D!\n");
      return;
    }
    machine_.mutable_cpu_state()->interrupt_enabled = value;
    machine_.mutable_cpu_state()->interrupt_enable_next = false;
  } else {
    if (!value_arg.is_word) {
      printf("Error: Value must be a number!\n");
      return;
    }
    if (reg_name == "PC") {
      machine_.mutable_cpu_state()->program_counter =
          Machine::TrimAddress(value_arg.value_word);
    } else if (reg_name == "IL") {
      machine_.mutable_cpu_state()->interrupt_link =
          Machine::TrimAddress(value_arg.value_word);
    } else {
      CpuState::RegisterId reg_id;
      if (!CpuState::RegisterNameToId(reg_name, &reg_id)) {
        printf("Error: Invalid register name '%s'!\n", reg_name.c_str());
        return;
      }
      machine_.mutable_cpu_state()->registers[reg_id] =
          Machine::TrimWord(value_arg.value_word);
    }
  }
}

}  // namespace simulator
}  // namespace sicxe
