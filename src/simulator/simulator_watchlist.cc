#include "simulator/simulator.h"

#include <memory>
#include <utility>
#include "common/float_util.h"

using sicxe::machine::Machine;
using std::make_pair;
using std::string;
using std::unique_ptr;

namespace sicxe {
namespace simulator {

const int Simulator::kVariableNameMaxLength = 12;

void Simulator::CommandWatchlistPrint(const CommandInterface::ParsedArgumentMap&) {
  // renumber list
  if (variable_next_number_ != static_cast<int>(variables_.size())) {
    variable_next_number_ = 0;
    variable_number_map_.clear();
    for (auto& variable : variables_) {
      variable->number = variable_next_number_++;
      variable->number_map_iter = variable_number_map_.insert(
          make_pair(variable->number, variable.get())).first;
    }
  }

  if (variables_.empty()) {
    return;
  }
  printf(" %-8s %-12s %-8s %-12s\n", "NUMBER", "NAME", "ADDRESS", "VALUE");
  for (const auto& variable : variables_) {
    string name = variable->name;
    if (name.empty()) {
      name = "[no name]";
    }
    printf(" %-8d %-12s %06x%-2s ", variable->number, name.c_str(),
           variable->address, "");
    if (variable->type == Variable::BYTE) {
      uint8 value = machine_.ReadMemoryByte(variable->address);
      printf("%02x%-6s %-10u ", static_cast<uint32>(value), "",
             static_cast<uint32>(value));
      if (value >= 0x20 && value <= 0x7E) {
        printf("'%c'", value);
      }
    } else if (variable->type == Variable::WORD) {
      uint32 value = machine_.ReadMemoryWord(variable->address);
      printf("%06x   %-10u %-10d", value, value, Machine::SignExtendWord(value));
    } else if (variable->type == Variable::ASCII) {
      uint32 address = variable->address;
      printf("\"");
      for (int i = 0; i < 40; i++, address++) {
        uint8 value = machine_.ReadMemoryByte(address);
        if (!(value >= 0x20 && value <= 0x7E)) {
          break;
        }
        printf("%c", value);
      }
      printf("\"");
    } else if (variable->type == Variable::FLOAT) {
      uint8 float_data[6];
      machine_.ReadMemoryFloat(variable->address, float_data);
      double value = FloatUtil::DecodeFloatData(float_data);
      printf("%g", value);
    }
    printf("\n");
  }
}

void Simulator::CommandWatchlistClear(const CommandInterface::ParsedArgumentMap&) {
  variable_number_map_.clear();
  variable_name_map_.clear();
  variables_.clear();
  variable_next_number_ = 0;
}

void Simulator::CommandWatchlistAdd(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  string name;
  {
    auto it = arguments.find("name");
    if (it != arguments.end()) {
      const CommandInterface::ParsedArgument& name_arg = it->second;
      name = name_arg.value_str;
      if (name.size() > kVariableNameMaxLength) {
        printf("Error: Name length is limited to %d characters!\n",
               kVariableNameMaxLength);
        return;
      }
    }
  }
  const CommandInterface::ParsedArgument& type_arg = arguments.find("type")->second;
  Variable::TypeId type = Variable::BYTE;
  if (type_arg.value_str == "byte") {
    type = Variable::BYTE;
  } else if (type_arg.value_str == "word") {
    type = Variable::WORD;
  } else if (type_arg.value_str == "ascii") {
    type = Variable::ASCII;
  } else if (type_arg.value_str == "float") {
    type = Variable::FLOAT;
  } else {
    printf("Error: Type must be 'byte', 'word', 'ascii' or 'float'!\n");
    return;
  }

  if (!name.empty() &&
      variable_name_map_.find(name) != variable_name_map_.end()) {
    printf("Error: A variable with this name already exists!\n");
    return;
  }

  variables_.emplace_back(unique_ptr<Variable>(new Variable));
  Variable* variable = variables_.back().get();
  variable->number = variable_next_number_++;
  variable->address = address;
  variable->name = name;
  variable->type = type;
  variable->list_iter = --variables_.end();
  variable->number_map_iter =
    variable_number_map_.insert(make_pair(variable->number, variable)).first;
  if (!name.empty()) {
    variable->name_map_iter =
      variable_name_map_.insert(make_pair(variable->name, variable)).first;
  }
}

Simulator::Variable* Simulator::FindVariable(
    const CommandInterface::ParsedArgumentMap& arguments) {
  int arg_count = 0;
  const CommandInterface::ParsedArgument* number_arg = nullptr;
  const CommandInterface::ParsedArgument* name_arg = nullptr;
  {
    auto it = arguments.find("number");
    if (it != arguments.end()) {
      number_arg = &it->second;
      arg_count++;
    }
  }
  {
    auto it = arguments.find("name");
    if (it != arguments.end()) {
      name_arg = &it->second;
      arg_count++;
    }
  }
  if (arg_count == 0) {
    printf("Error: Please specify 'number' or 'name'!\n");
    return nullptr;
  }
  if (arg_count > 1) {
    printf("Error: Please specify only one of 'number' or 'name'!\n");
    return nullptr;
  }

  Variable* variable = nullptr;
  if (number_arg != nullptr) {
    if (!number_arg->is_word) {
      printf("Error: Argument 'number' must be a number!\n");
      return nullptr;
    }

    auto it = variable_number_map_.find(number_arg->value_word);
    if (it == variable_number_map_.end()) {
      printf("Error: No variable with such number!\n");
      return nullptr;
    }
    variable = it->second;
  } else {
    auto it = variable_name_map_.find(name_arg->value_str);
    if (it == variable_name_map_.end()) {
      printf("Error: No variable with such name!\n");
      return nullptr;
    }
    variable = it->second;
  }
  return variable;
}

void Simulator::CommandWatchlistRemove(
    const CommandInterface::ParsedArgumentMap& arguments) {
  Variable* variable = FindVariable(arguments);
  if (variable == nullptr) {
    return;
  }

  variable_number_map_.erase(variable->number_map_iter);
  if (!variable->name.empty()) {
    variable_name_map_.erase(variable->name_map_iter);
  }
  variables_.erase(variable->list_iter);
}

void Simulator::CommandWatchlistWrite(
    const CommandInterface::ParsedArgumentMap& arguments) {
  Variable* variable = FindVariable(arguments);
  if (variable == nullptr) {
    return;
  }

  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (variable->type == Variable::BYTE) {
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
    machine_.WriteMemoryByte(variable->address, value);
  } else if (variable->type == Variable::WORD) {
    if (!value_arg.is_word) {
      printf("Error: Value must be a number!\n");
      return;
    }
    machine_.WriteMemoryWord(variable->address, value_arg.value_word);
  } else if (variable->type == Variable::ASCII) {
    machine_.WriteMemory(variable->address, value_arg.value_str.size() + 1,
        reinterpret_cast<const uint8*>(value_arg.value_str.c_str()));
  } else if (variable->type == Variable::FLOAT) {
    const char* str_ptr = value_arg.value_str.c_str();
    char* end_ptr = nullptr;
    double value = strtod(str_ptr, &end_ptr);
    if (end_ptr != str_ptr + value_arg.value_str.size()) {
      printf("Error: Value must be a floating point number!\n");
      return;
    }
    uint8 float_data[6];
    FloatUtil::EncodeFloatData(value, float_data);
    machine_.WriteMemoryFloat(variable->address, float_data);
  }
}

}  // namespace simulator
}  // namespace sicxe
