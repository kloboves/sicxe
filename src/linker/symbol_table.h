#ifndef LINKER_SYMBOL_TABLE_H
#define LINKER_SYMBOL_TABLE_H

#include <list>
#include <map>
#include <string>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class ObjectFile;

namespace linker {

class SymbolTable {
 public:
  DISALLOW_COPY_AND_MOVE(SymbolTable);

  enum TypeId {
    UNDEFINED = 0,
    DEFINED = 1
  };

  struct Entry {
    Entry();

    TypeId type;
    uint32 address;
    const ObjectFile* file_defined;
    std::list<const ObjectFile*> files_referenced;
  };

  typedef std::map<std::string, Entry> EntryMap;

  SymbolTable();
  ~SymbolTable();

  const Entry* Find(const std::string& symbol_name) const;
  Entry* FindOrCreateNew(const std::string& symbol_name);

  const EntryMap& entry_map() const;
  EntryMap* mutable_entry_map();

 private:
  EntryMap entry_map_;
};

}  // namespace linker
}  // namespace sicxe

#endif  // LINKER_SYMBOL_TABLE_H
