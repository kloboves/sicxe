#ifndef MACHINE_LOGIC_H
#define MACHINE_LOGIC_H

#include "common/macros.h"
#include "machine/execute_result.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

class InstructionLogic {
 public:
  DISALLOW_COPY_AND_MOVE(InstructionLogic);

  static bool IsImmediateAddressing(const InstructionInstance& instance);

  InstructionLogic();
  virtual ~InstructionLogic();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const = 0;
};

}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_H
