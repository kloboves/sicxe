#include "linker/symbol_table.h"

using std::string;

namespace sicxe {
namespace linker {

SymbolTable::Entry::Entry() : type(UNDEFINED), address(0), file_defined(nullptr) {}

SymbolTable::SymbolTable() {}
SymbolTable::~SymbolTable() {}

const SymbolTable::Entry* SymbolTable::Find(const string& symbol_name) const  {
  auto it = entry_map_.find(symbol_name);
  if (it == entry_map_.end()) {
    return nullptr;
  }
  return &it->second;
}

SymbolTable::Entry* SymbolTable::FindOrCreateNew(const string& symbol_name) {
  return &entry_map_[symbol_name];
}

const SymbolTable::EntryMap& SymbolTable::entry_map() const {
  return entry_map_;
}

SymbolTable::EntryMap* SymbolTable::mutable_entry_map() {
  return &entry_map_;
}

}  // namespace linker
}  // namespace sicxe
