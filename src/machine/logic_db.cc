#include "machine/logic_db.h"

#include <assert.h>
#include "machine/logic.h"

namespace sicxe {
namespace machine {

LogicDB::LogicDB() {}
LogicDB::~LogicDB() {}

void LogicDB::Register(uint8 opcode, InstructionLogic* logic) {
  assert(logic_table_[opcode].get() == nullptr);
  logic_table_[opcode].reset(logic);
}

const InstructionLogic* LogicDB::Find(uint8 opcode) const {
  return logic_table_[opcode].get();
}

}  // namespace machine
}  // namespace sicxe
