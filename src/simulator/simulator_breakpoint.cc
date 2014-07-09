#include "simulator/simulator.h"

#include <memory>
#include <utility>

using std::make_pair;
using std::string;
using std::unique_ptr;

namespace sicxe {
namespace simulator {

void Simulator::CommandBreakpointOn(
    const CommandInterface::ParsedArgumentMap&) {
  breakpoints_enable_ = true;
}

void Simulator::CommandBreakpointOff(
    const CommandInterface::ParsedArgumentMap&) {
  breakpoints_enable_ = false;
}

void Simulator::CommandBreakpointClear(const CommandInterface::ParsedArgumentMap&) {
  breakpoint_number_map_.clear();
  breakpoint_address_map_.clear();
  breakpoint_name_map_.clear();
  breakpoints_.clear();
  breakpoint_next_number_ = 0;
}

void Simulator::CommandBreakpointPrint(const CommandInterface::ParsedArgumentMap&) {
  // renumber list
  if (breakpoint_next_number_ != static_cast<int>(breakpoints_.size())) {
    breakpoint_next_number_ = 0;
    breakpoint_number_map_.clear();
    for (auto& breakpoint : breakpoints_) {
      breakpoint->number = breakpoint_next_number_++;
      breakpoint->number_map_iter = breakpoint_number_map_.insert(
          make_pair(breakpoint->number, breakpoint.get())).first;
    }
  }

  if (breakpoints_.empty()) {
    return;
  }
  printf(" %-8s %-12s %-12s\n", "NUMBER", "ADDRESS", "NAME");
  for (const auto& breakpoint : breakpoints_) {
    printf(" %-8d %06x%-6s ", breakpoint->number, breakpoint->address, "");
    if (!breakpoint->name.empty()) {
      printf("%s", breakpoint->name.c_str());
    } else {
      printf("[no name]");
    }
    printf("\n");
  }
}

void Simulator::CommandBreakpointAdd(
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
      // TODO: Maybe add a maximum name length limit
      name = name_arg.value_str;
    }
  }

  if (breakpoint_address_map_.find(address) != breakpoint_address_map_.end()) {
    printf("Error: A breakpoint already exists at this address!\n");
    return;
  }
  if (!name.empty() &&
      breakpoint_name_map_.find(name) != breakpoint_name_map_.end()) {
    printf("Error: A breakpoint with this name already exists!\n");
    return;
  }

  breakpoints_.emplace_back(unique_ptr<Breakpoint>(new Breakpoint));
  Breakpoint* breakpoint = breakpoints_.back().get();
  breakpoint->number = breakpoint_next_number_++;
  breakpoint->address = address;
  breakpoint->name = name;
  breakpoint->list_iter = --breakpoints_.end();
  breakpoint->number_map_iter =
    breakpoint_number_map_.insert(make_pair(breakpoint->number, breakpoint)).first;
  breakpoint->address_map_iter =
    breakpoint_address_map_.insert(make_pair(breakpoint->address, breakpoint)).first;
  if (!name.empty()) {
    breakpoint->name_map_iter =
      breakpoint_name_map_.insert(make_pair(breakpoint->name, breakpoint)).first;
  }
}

void Simulator::CommandBreakpointRemove(
    const CommandInterface::ParsedArgumentMap& arguments) {
  int arg_count = 0;
  const CommandInterface::ParsedArgument* number_arg = nullptr;
  const CommandInterface::ParsedArgument* address_arg = nullptr;
  const CommandInterface::ParsedArgument* name_arg = nullptr;
  {
    auto it = arguments.find("number");
    if (it != arguments.end()) {
      number_arg = &it->second;
      arg_count++;
    }
  }
  {
    auto it = arguments.find("address");
    if (it != arguments.end()) {
      address_arg = &it->second;
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
    printf("Error: Please specify 'number', 'address' or 'name'!\n");
    return;
  }
  if (arg_count > 1) {
    printf("Error: Please specify only one of 'number', 'address' or 'name'!\n");
    return;
  }

  Breakpoint* breakpoint = nullptr;
  if (number_arg != nullptr) {
    if (!number_arg->is_word) {
      printf("Error: Argument 'number' must be a number!\n");
      return;
    }
    auto it = breakpoint_number_map_.find(number_arg->value_word);
    if (it == breakpoint_number_map_.end()) {
      printf("Error: No breakpoint with such number!\n");
      return;
    }
    breakpoint = it->second;
  } else if (address_arg != nullptr) {
    if (!address_arg->is_word) {
      printf("Error: Address must be a number!\n");
      return;
    }
    auto it = breakpoint_address_map_.find(address_arg->value_word);
    if (it == breakpoint_address_map_.end()) {
      printf("Error: No breakpoint with such address!\n");
      return;
    }
    breakpoint = it->second;
  } else {
    auto it = breakpoint_name_map_.find(name_arg->value_str);
    if (it == breakpoint_name_map_.end()) {
      printf("Error: No breakpoint with such name!\n");
      return;
    }
    breakpoint = it->second;
  }

  breakpoint_number_map_.erase(breakpoint->number_map_iter);
  breakpoint_address_map_.erase(breakpoint->address_map_iter);
  if (!breakpoint->name.empty()) {
    breakpoint_name_map_.erase(breakpoint->name_map_iter);
  }
  breakpoints_.erase(breakpoint->list_iter);
}

}  // namespace simulator
}  // namespace sicxe
