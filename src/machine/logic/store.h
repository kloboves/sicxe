#ifndef MACHINE_LOGIC_STORE_H
#define MACHINE_LOGIC_STORE_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class StoreWord : public InstructionLogic {
 public:
  explicit StoreWord(CpuState::RegisterId implicit_register);
  ~StoreWord();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
 private:
  CpuState::RegisterId implicit_register_;
};

class StoreByte : public InstructionLogic {
 public:
  StoreByte();
  ~StoreByte();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class StoreFlags : public InstructionLogic {
 public:
  StoreFlags();
  ~StoreFlags();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_STORE_H
