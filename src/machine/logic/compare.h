#ifndef MACHINE_LOGIC_COMPARE_H
#define MACHINE_LOGIC_COMPARE_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class CompareMem : public InstructionLogic {
 public:
  CompareMem();
  ~CompareMem();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class CompareReg : public InstructionLogic {
 public:
  CompareReg();
  ~CompareReg();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class IncrementCompareMem : public InstructionLogic {
 public:
  IncrementCompareMem();
  ~IncrementCompareMem();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class IncrementCompareReg : public InstructionLogic {
 public:
  IncrementCompareReg();
  ~IncrementCompareReg();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_COMPARE_H
