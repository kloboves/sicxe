#ifndef ASSEMBLER_SYMBOL_TABLE_H
#define ASSEMBLER_SYMBOL_TABLE_H

#include <map>
#include <string>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class TextFile;

namespace assembler {

class BlockTable;

class SymbolTable {
 public:
  DISALLOW_COPY_AND_MOVE(SymbolTable);

  enum TypeId {
    UNKNOWN = 0,
    INTERNAL,
    EXTERNAL
  };

  struct Entry {
    Entry();

    TypeId type;

    // applicable for all symbols:
    bool referenced;

    // applicable for internal and external symbols:
    bool relative;  // true=relative, false=absolute; external, exported always relative

    // applicable for internal symbols only:
    bool defined;  // symbol can be exported and thus internal, but not defined yet
    bool resolved;  // symbol can be defined, but actual address not yet known (blocks)
    bool exported;

    uint32 address;  // actual address of symbol in this module
    int block;  // -1 = no block, 0 = default block (symbols resolved when defined)
    uint32 block_address;  // address in block (only for symbols within blocks)
  };

  typedef std::map<std::string, Entry> EntryMap;

  SymbolTable();
  ~SymbolTable();

  const Entry* Find(const std::string& symbol_name) const;
  Entry* FindOrCreateNew(const std::string& symbol_name);

  void OutputToTextFile(const BlockTable* block_table, TextFile* file) const;

  const EntryMap& entry_map() const;
  EntryMap* mutable_entry_map();

 private:
   EntryMap entry_map_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_SYMBOL_TABLE_H
