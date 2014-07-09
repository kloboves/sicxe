#ifndef MACHINE_LOGIC_SHIFT_H
#define MACHINE_LOGIC_SHIFT_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class Shift : public InstructionLogic {
 public:
  explicit Shift(bool direction_right);
  ~Shift();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;

 private:
  bool direction_right_;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_SHIFT_H
