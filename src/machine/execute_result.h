#ifndef MACHINE_EXECUTE_RESULT_H
#define MACHINE_EXECUTE_RESULT_H

namespace sicxe {
namespace machine {

class ExecuteResult {
 public:
  enum ResultId {
    OK = 0,
    DEVICE_ERROR,
    INVALID_OPCODE,
    INVALID_ADDRESSING,
    NOT_IMPLEMENTED,
    ENDLESS_LOOP
  };
};

}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_EXECUTE_RESULT_H
