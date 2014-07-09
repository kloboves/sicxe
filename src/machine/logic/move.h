#ifndef MACHINE_LOGIC_MOVE_H
#define MACHINE_LOGIC_MOVE_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class ClearReg : public InstructionLogic {
 public:
  ClearReg();
  ~ClearReg();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class MoveReg : public InstructionLogic {
 public:
  MoveReg();
  ~MoveReg();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_MOVE_H
