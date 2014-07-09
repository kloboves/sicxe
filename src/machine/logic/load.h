#ifndef MACHINE_LOGIC_LOAD_H
#define MACHINE_LOGIC_LOAD_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class LoadWord : public InstructionLogic {
 public:
  explicit LoadWord(CpuState::RegisterId implicit_register);
  ~LoadWord();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
 private:
  CpuState::RegisterId implicit_register_;
};

class LoadByte : public InstructionLogic {
 public:
  LoadByte();
  ~LoadByte();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_LOAD_H
