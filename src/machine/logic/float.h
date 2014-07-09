#ifndef MACHINE_LOGIC_FLOAT_H
#define MACHINE_LOGIC_FLOAT_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class FloatToInt : public InstructionLogic {
 public:
  FloatToInt();
  ~FloatToInt();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class IntToFloat : public InstructionLogic {
 public:
  IntToFloat();
  ~IntToFloat();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class FloatArithmetic : public InstructionLogic {
 public:
  enum OperationId {
    ADD = 0,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
  };

  explicit FloatArithmetic(OperationId operation);
  ~FloatArithmetic();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;

 private:
  OperationId operation_;
};

class FloatCompare : public InstructionLogic {
 public:
  FloatCompare();
  ~FloatCompare();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class LoadFloat : public InstructionLogic {
 public:
  LoadFloat();
  ~LoadFloat();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

class StoreFloat : public InstructionLogic {
 public:
  StoreFloat();
  ~StoreFloat();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_FLOAT_H
