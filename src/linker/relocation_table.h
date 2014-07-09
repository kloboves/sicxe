#ifndef LINKER_RELOCATION_TABLE_H
#define LINKER_RELOCATION_TABLE_H

#include <map>
#include <string>
#include <vector>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {
namespace linker {

class RelocationTable {
 public:
  DISALLOW_COPY_AND_MOVE(RelocationTable);

  enum TypeId {
    SIMPLE = 0,
    SYMBOL,
  };

  struct Entry {
    Entry();

    TypeId type;
    uint8 nibbles;
    std::vector<std::pair<bool, std::string> > symbols;  // sign and symbol name
  };

  typedef std::map<uint32, Entry> EntryMap;

  RelocationTable();
  ~RelocationTable();

  const Entry* FindRelocation(uint32 address) const;

  bool AddRelocationSimple(uint32 address, uint8 nibbles);
  bool AddRelocationSymbol(uint32 address, uint8 nibbles, bool sign,
                           const std::string& symbol_name);

  const EntryMap& entry_map() const;
  EntryMap* mutable_entry_map();

 private:
  EntryMap entry_map_;
};

}  // namespace linker
}  // namespace sicxe

#endif  // LINKER_RELOCATION_TABLE_H
