#include "linker/relocation_table.h"

using std::make_pair;
using std::pair;

namespace sicxe {
namespace linker {

RelocationTable::Entry::Entry() : type(SIMPLE), nibbles(0) {}

RelocationTable::RelocationTable() {}
RelocationTable::~RelocationTable() {}

const RelocationTable::Entry* RelocationTable::FindRelocation(uint32 address) const {
  auto it = entry_map_.find(address);
  if (it == entry_map_.end()) {
    return nullptr;
  }
  return &it->second;
}

bool RelocationTable::AddRelocationSimple(uint32 address, uint8 nibbles) {
  auto rv = entry_map_.insert(make_pair(address, Entry()));
  if (!rv.second) {
    // already exists
    return false;
  }
  Entry* entry = &rv.first->second;
  entry->type = SIMPLE;
  entry->nibbles = nibbles;
  return true;
}

bool RelocationTable::AddRelocationSymbol(uint32 address, uint8 nibbles, bool sign,
                                          const std::string& symbol_name) {
  auto rv = entry_map_.insert(make_pair(address, Entry()));
  Entry* entry = &rv.first->second;
  if (rv.second) {
    entry->type = SYMBOL;
    entry->nibbles = nibbles;
  } else if (entry->type != SYMBOL || entry->nibbles != nibbles) {
    return false;
  }
  entry->symbols.push_back(make_pair(sign, symbol_name));
  return true;
}

const RelocationTable::EntryMap& RelocationTable::entry_map() const {
  return entry_map_;
}

RelocationTable::EntryMap* RelocationTable::mutable_entry_map() {
  return &entry_map_;
}


}  // namespace linker
}  // namespace sicxe
