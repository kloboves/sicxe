#ifndef MACHINE_LOGIC_JUMP_H
#define MACHINE_LOGIC_JUMP_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class Jump : public InstructionLogic {
 public:
  explicit Jump(bool save_program_counter);
  ~Jump();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;

 private:
  bool save_program_counter_;
};

class JumpConditional : public InstructionLogic {
 public:
  explicit JumpConditional(CpuState::ConditionId condition);
  ~JumpConditional();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;

 private:
  CpuState::ConditionId condition_;
};

class Return : public InstructionLogic {
 public:
  Return();
  ~Return();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_JUMP_H
