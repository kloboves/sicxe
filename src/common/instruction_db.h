#ifndef COMMON_INSTRUCTION_DB_H
#define COMMON_INSTRUCTION_DB_H

#include <map>
#include <memory>
#include <string>
#include "common/format.h"
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class ErrorDB;
class Instruction;
class TextFile;

class InstructionDB {
 public:
  DISALLOW_COPY_AND_MOVE(InstructionDB);

  InstructionDB();
  ~InstructionDB();

  void Clear();

  // Add an Instruction to the database (takes ownership).
  void Register(Instruction* instruction);

  bool IsOpcodeAvailable(uint8 opcode, Format::FormatId format) const;

  const Instruction* FindOpcode(uint8 opcode) const;
  const Instruction* FindMnemonic(const std::string& mnemonic) const;

  // Read instruction database from a TextFile
  static bool LoadFromFile(const TextFile& file, InstructionDB* instruction_db,
                           ErrorDB* error_db);

  // Returns a singleton with default instruction set.
  static const InstructionDB* Default();

 private:
  bool unavailable_mask_[1 << 6];
  std::unique_ptr<Instruction> instructions_[1 << 8];
  std::map<std::string, Instruction*> mnemonic_map_;
};

}  // namespace sicxe

#endif  // COMMON_INSTRUCTION_DB_H
