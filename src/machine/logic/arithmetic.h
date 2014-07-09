#ifndef MACHINE_LOGIC_ARITHMETIC_H
#define MACHINE_LOGIC_ARITHMETIC_H

#include "common/cpu_state.h"
#include "machine/logic.h"

namespace sicxe {

struct InstructionInstance;

namespace machine {

class Machine;

namespace logic {

class ArithmeticOperation {
 public:
  enum OperationId {
    ADD = 0,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    AND,
    OR,
    XOR
  };
};

class ArithmeticMem : public InstructionLogic {
 public:
  explicit ArithmeticMem(ArithmeticOperation::OperationId operation);
  ~ArithmeticMem();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;

 private:
  ArithmeticOperation::OperationId operation_;
};

class ArithmeticReg : public InstructionLogic {
 public:
  explicit ArithmeticReg(ArithmeticOperation::OperationId operation);
  ~ArithmeticReg();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;

 private:
  ArithmeticOperation::OperationId operation_;
};

class RegisterNegate : public InstructionLogic {
 public:
  explicit RegisterNegate();
  ~RegisterNegate();

  virtual ExecuteResult::ResultId Execute(const InstructionInstance& instance,
                                          Machine* machine) const;
};

}  // namespace logic
}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_ARITHMETIC_H
