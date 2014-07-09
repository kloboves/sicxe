#include "simulator/simulator.h"

#include <stdio.h>
#include <memory>
#include <string>
#include "common/float_util.h"

using sicxe::machine::Machine;
using std::string;

namespace sicxe {
namespace simulator {

const int Simulator::kMemoryPrintDefaultByteCount = 8 * 16;
const int Simulator::kMemoryPrintDefaultWordCount = 10;
const int Simulator::kMemoryPrintDefaultFloatCount = 10;
const int Simulator::kMemoryPrintMaxByteCount = 1000 * 16;
const int Simulator::kMemoryPrintMaxWordCount = 1000;
const int Simulator::kMemoryPrintMaxFloatCount = 1000;

void Simulator::CommandMemoryWriteByte(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  uint8 value = 0;
  if (value_arg.value_str.size() == 3 &&
      value_arg.value_str[0] == '\'' &&
      value_arg.value_str[2] == '\'') {
    value = static_cast<uint8>(value_arg.value_str[1]);
  } else {
    if (!value_arg.is_word) {
      printf("Error: Value must be a number or ASCII character in single quotes!\n");
      return;
    }
    value = value_arg.value_word & 0xFF;
  }
  machine_.WriteMemoryByte(address, value);
}

void Simulator::CommandMemoryWriteWord(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  if (!value_arg.is_word) {
    printf("Error: Value must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  uint32 value = value_arg.value_word;
  machine_.WriteMemoryWord(address, value);
}

void Simulator::CommandMemoryWriteFloat(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  const char* str_ptr = value_arg.value_str.c_str();
  char* end_ptr = nullptr;
  double value = strtod(str_ptr, &end_ptr);
  if (end_ptr != str_ptr + value_arg.value_str.size()) {
    printf("Error: Value must be a floating point number!\n");
    return;
  }
  uint32 address = Machine::TrimAddress(address_arg.value_word);
  uint8 float_data[6];
  FloatUtil::EncodeFloatData(value, float_data);
  machine_.WriteMemoryFloat(address, float_data);
}

void Simulator::CommandMemoryWriteAscii(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  bool null_terminate = true;
  {
    auto it = arguments.find("terminate");
    if (it != arguments.end()) {
      const CommandInterface::ParsedArgument& null_term_arg = it->second;
      if (!null_term_arg.is_bool) {
        printf("Error: Terminate must be a boolean!\n");
        return;
      }
      null_terminate = null_term_arg.value_bool;
    }
  }
  if (null_terminate) {
    machine_.WriteMemory(address, value_arg.value_str.size() + 1,
        reinterpret_cast<const uint8*>(value_arg.value_str.c_str()));
  } else {
    machine_.WriteMemory(address, value_arg.value_str.size(),
        reinterpret_cast<const uint8*>(value_arg.value_str.data()));
  }
}

void Simulator::CommandMemoryPrintBytes(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  int count = kMemoryPrintDefaultByteCount;
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
  if (count > kMemoryPrintMaxByteCount) {
    printf("Error: Count must be less than or equal to %d!\n", kMemoryPrintMaxByteCount);
    return;
  }
  if (address + count > (0x0FFFFF + 1)) {
    count = 0x0FFFFF - address + 1;
  }
  if (count <= 0) {
    return;
  }
  uint32 row_address = (address / 16) * 16;
  for (; row_address < address + count; row_address += 16) {
    printf(" %06x", row_address);
    for (int i = 0; i < 16; i++) {
      if (i == 0 || i == 8) {
        printf(" ");
      }
      printf(" ");
      uint32 a = row_address + i;
      if (a >= address && a < address + count) {
        uint8 value = machine_.ReadMemoryByte(a);
        printf("%02x", static_cast<uint32>(value));
      } else {
        printf("  ");
      }
    }

    printf("  |");
    for (int i = 0; i < 16; i++) {
      uint32 a = row_address + i;
      if (a >= address && a < address + count) {
        uint8 value = machine_.ReadMemoryByte(a);
        if (value >= 0x20 && value <= 0x7E) {
          printf("%c", value);
        } else {
          printf(".");
        }
      } else {
        printf(" ");
      }
    }
    printf("|\n");
  }
}

void Simulator::CommandMemoryPrintWords(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  int count = kMemoryPrintDefaultWordCount;
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
  if (count > kMemoryPrintMaxWordCount) {
    printf("Error: Count must be less than or equal to %d!\n", kMemoryPrintMaxWordCount);
    return;
  }
  if (address + 3 * count > 0x0FFFFF) {
    count = (0x0FFFFF - address + 1) / 3;
  }
  if (count <= 0) {
    return;
  }
  printf(" %-12s %-12s %-12s %-12s\n",
         "ADDRESS", "HEX", "UNSIGNED", "SIGNED");
  uint32 word_address = address;
  for (int i = 0; i < count; i++, word_address += 3) {
    uint32 value = machine_.ReadMemoryWord(word_address);
    printf(" %06x%-6s %06x%-6s %-12u %-12d\n", word_address, "",
           value, "", value, Machine::SignExtendWord(value));
  }
}

void Simulator::CommandMemoryPrintFloats(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  int count = kMemoryPrintDefaultFloatCount;
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
  if (count > kMemoryPrintMaxFloatCount) {
    printf("Error: Count must be less than or equal to %d!\n", kMemoryPrintMaxWordCount);
    return;
  }
  if (address + 6 * count > 0x0FFFFF) {
    count = (0x0FFFFF - address + 1) / 6;
  }
  if (count <= 0) {
    return;
  }
  printf(" %-12s %-16s %-12s\n",
         "ADDRESS", "HEX", "VALUE");
  uint32 word_address = address;
  for (int i = 0; i < count; i++, word_address += 6) {
    printf(" %06x%-6s ", word_address, "");
    uint8 float_data[6];
    machine_.ReadMemoryFloat(word_address, float_data);
    for (int j = 0; j < 6; j++) {
      printf("%02x", static_cast<uint32>(float_data[j]) & 0xff);
    }
    double value = FloatUtil::DecodeFloatData(float_data);
    printf("%-4s %g\n", "", value);
  }
}

}  // namespace simulator
}  // namespace sicxe
