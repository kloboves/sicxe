#ifndef COMMON_CPU_STATE_H
#define COMMON_CPU_STATE_H

#include "common/types.h"

#include <string>

namespace sicxe {

struct CpuState {
  enum RegisterId {
    REG_A = 0,
    REG_X = 1,
    REG_L = 2,
    REG_B = 3,
    REG_S = 4,
    REG_T = 5,

    NUM_REGISTERS  // keep last in enum
  };

  enum ConditionId {
    LESS = 0,
    EQUAL = 1,
    GREATER = 2
  };

  static bool RegisterNameToId(const std::string& name, RegisterId* id);
  static bool RegisterIdToName(RegisterId id, std::string* name);

  CpuState();
  ~CpuState();

  // main registers
  uint32 program_counter;
  uint32 registers[NUM_REGISTERS];
  uint8 float_register[6];
  ConditionId condition_code;

  // interrupt registers
  bool interrupt_enabled;
  uint32 interrupt_link;
  ConditionId interrupt_condition_code;

  // hidden registers
  uint32 target_address;
  bool interrupt_enable_next;  // enable interrupts on next instruction
};

}  // namespace sicxe

#endif  // COMMON_CPU_STATE_H
