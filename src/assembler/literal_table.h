#ifndef ASSEMBLER_LITERAL_TABLE_H
#define ASSEMBLER_LITERAL_TABLE_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class TextFile;

namespace assembler {

class LiteralTable {
 public:
  DISALLOW_COPY_AND_MOVE(LiteralTable);

  enum TypeId {
    BYTE = 0,
    WORD,
    FLOAT
  };

  struct Entry {
    Entry(int the_id, TypeId the_type);

    int id;
    TypeId type;
    union {
      uint8 byte;
      uint32 word;
      uint64 float_bin;
    } value;
  };

  typedef std::vector<std::unique_ptr<Entry> > EntryVector;

  LiteralTable();
  ~LiteralTable();

  static std::string LiteralSymbolName(int literal_id);

  const Entry* GetLiteral(int literal_id) const;
  int NewLiteralByte(uint8 value);
  int NewLiteralByte(const std::string& data);
  int NewLiteralWord(uint32 value);
  int NewLiteralWord(const std::string& data);
  int NewLiteralFloat(const std::string& data);

  // Clears deduplication lookup tables - should be called when literals are emitted
  void ClearLookupTables();

  void OutputToTextFile(TextFile* file) const;

  const EntryVector& entries() const;

 private:
  EntryVector entries_;
  std::map<uint8, Entry*> byte_map_;
  std::map<uint32, Entry*> word_map_;
  std::map<uint64, Entry*> float_map_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_LITERAL_TABLE_H
