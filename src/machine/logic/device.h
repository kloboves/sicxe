#ifndef MACHINE_LOGIC_DEVICE_H
#define MACHINE_LOGIC_DEVICE_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class TestDevice : public InstructionLogic {
 public:
  TestDevice();
  ~TestDevice();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class ReadDevice : public InstructionLogic {
 public:
  ReadDevice();
  ~ReadDevice();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class WriteDevice : public InstructionLogic {
 public:
  WriteDevice();
  ~WriteDevice();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_DEVICE_H
