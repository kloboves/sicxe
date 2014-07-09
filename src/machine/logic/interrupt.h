#ifndef MACHINE_LOGIC_INTERRUPT_H
#define MACHINE_LOGIC_INTERRUPT_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class InterruptEnable : public InstructionLogic {
 public:
  InterruptEnable();
  ~InterruptEnable();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class InterruptDisable : public InstructionLogic {
 public:
  InterruptDisable();
  ~InterruptDisable();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class InterruptReturn : public InstructionLogic {
 public:
  InterruptReturn();
  ~InterruptReturn();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class InterruptLinkStore : public InstructionLogic {
 public:
  InterruptLinkStore();
  ~InterruptLinkStore();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_INTERRUPT_H
