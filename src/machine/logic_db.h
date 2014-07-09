#ifndef MACHINE_LOGIC_DB_H
#define MACHINE_LOGIC_DB_H

#include <memory>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {
namespace machine {

class InstructionLogic;

class LogicDB {
 public:
  DISALLOW_COPY_AND_MOVE(LogicDB);

  LogicDB();
  ~LogicDB();

  // Adds an InstructionLogic to the database (takes ownership).
  void Register(uint8 opcode, InstructionLogic* logic);

  const InstructionLogic* Find(uint8 opcode) const;

  // Returns a singleton with logic for default instruction set.
  static const LogicDB* Default();

 private:
  std::unique_ptr<InstructionLogic> logic_table_[1 << 8];
};

}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOGIC_DB_H
