#include "simulator/simulator.h"

#include <stdio.h>
#include <string>

using sicxe::machine::ExecuteResult;
using std::string;

namespace sicxe {
namespace simulator {

const int Simulator::kStepMaxCount = 10000;

namespace {

void PrintExecuteResultError(ExecuteResult::ResultId result) {
  switch (result) {
    case ExecuteResult::ENDLESS_LOOP:
      printf("Endless loop\n");
      break;
    case ExecuteResult::DEVICE_ERROR:
      printf("Device error\n");
      break;
    case ExecuteResult::INVALID_OPCODE:
      printf("Invalid opcode\n");
      break;
    case ExecuteResult::INVALID_ADDRESSING:
      printf("Invalid addressing\n");
      break;
    case ExecuteResult::NOT_IMPLEMENTED:
      printf("Not implemented\n");
      break;
    default:
      printf("Unknown error\n");
      break;
  }
}

}  // namespace

void Simulator::CommandStart(const CommandInterface::ParsedArgumentMap&) {
  uint64 instruction_count = 0;
  bool instruction_count_overflow = false;
  bool cancelled = false;
  bool breakpoint = false;
  string breakpoint_name;
  ExecuteResult::ResultId result = ExecuteResult::OK;
  command_interface_.StartCancellableAction();
  while (true) {
    if (command_interface_.ActionIsCancelled()) {
      cancelled = true;
      break;
    }

    if (breakpoints_enable_) {
      uint32 program_counter = machine_.cpu_state().program_counter;
      auto it = breakpoint_address_map_.find(program_counter);
      bool ignore = false;
      if (breakpoint_last_hit_address_ == program_counter) {
        ignore = true;
      }
      breakpoint_last_hit_address_ = 0xFFFFFF;
      if (!ignore && it != breakpoint_address_map_.end()) {
        breakpoint = true;
        breakpoint_name = it->second->name;
        breakpoint_last_hit_address_ = program_counter;
        break;
      }
    }

    if ((result = machine_.Execute()) != ExecuteResult::OK) {
      break;
    }
    instruction_count++;
    if (instruction_count == 0) {
      instruction_count_overflow = true;
    }
  }
  command_interface_.EndCancellableAction();
  if (cancelled) {
    printf("\n");
  }
  printf(" %06x  %-11s  ", machine_.cpu_state().program_counter, "");
  if (cancelled) {
    printf("Stopped by user\n");
  } else if (breakpoint) {
    printf("Breakpoint ");
    if (!breakpoint_name.empty()) {
      printf("%s", breakpoint_name.c_str());
    } else {
      printf("[no name]");
    }
    printf("\n");
  } else {
    PrintExecuteResultError(result);
  }
  printf(" Number of instructions executed: ");
  if (instruction_count_overflow) {
    printf("maximum\n");
  } else {
    printf("%llu\n", instruction_count);
  }
}

void Simulator::CommandStep(const CommandInterface::ParsedArgumentMap& arguments) {
  int count = 1;
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
  if (count > kStepMaxCount) {
    printf("Error: Count must be less than equal to %d!\n", kStepMaxCount);
    return;
  }

  bool breakpoint = false;
  string breakpoint_name;
  ExecuteResult::ResultId result = ExecuteResult::OK;
  for (int i = 0; i < count; i++) {
    if (breakpoints_enable_) {
      uint32 program_counter = machine_.cpu_state().program_counter;
      auto it = breakpoint_address_map_.find(program_counter);
      bool ignore = false;
      if (breakpoint_last_hit_address_ == program_counter) {
        ignore = true;
      }
      breakpoint_last_hit_address_ = 0xFFFFFF;
      if (!ignore && it != breakpoint_address_map_.end()) {
        breakpoint = true;
        breakpoint_name = it->second->name;
        breakpoint_last_hit_address_ = program_counter;
        break;
      }
    }

    if (auto_disassemble_) {
      DisassembleInstruction(machine_.cpu_state().program_counter);
    }

    if ((result = machine_.Execute()) != ExecuteResult::OK) {
      break;
    }
  }

  if (breakpoint || result != ExecuteResult::OK) {
    printf(" %06x  %-11s  ", machine_.cpu_state().program_counter, "");
    if (breakpoint) {
      printf("Breakpoint ");
      if (!breakpoint_name.empty()) {
        printf("%s", breakpoint_name.c_str());
      } else {
        printf("[no name]");
      }
      printf("\n");
    } else {
      PrintExecuteResultError(result);
    }
  }
}

void Simulator::CommandInterrupt(const CommandInterface::ParsedArgumentMap&) {
  if (!machine_.cpu_state().interrupt_enabled) {
    printf("Note: Interrupts are disabled.\n");
  }
  machine_.Interrupt();
}

}  // namespace simulator
}  // namespace sicxe
