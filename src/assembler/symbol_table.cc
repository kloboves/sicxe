#include "assembler/symbol_table.h"

#include <memory>
#include <string>
#include "assembler/block_table.h"
#include "common/text_file.h"

using std::string;
using std::unique_ptr;

namespace sicxe {
namespace assembler {

SymbolTable::Entry::Entry()
  : type(UNKNOWN), referenced(false), relative(false), defined(false),
    resolved(false), exported(false), address(0), block(-1), block_address(0) {}


SymbolTable::SymbolTable() {}
SymbolTable::~SymbolTable() {}

const SymbolTable::Entry* SymbolTable::Find(const std::string& symbol_name) const {
  auto it = entry_map_.find(symbol_name);
  if (it == entry_map_.end()) {
    return nullptr;
  }
  return &it->second;
}

SymbolTable::Entry* SymbolTable::FindOrCreateNew(const std::string& symbol_name) {
  return &entry_map_[symbol_name];
}

void SymbolTable::OutputToTextFile(const BlockTable* block_table, TextFile* file) const {
  size_t buffer_size = 200;
  unique_ptr<char[]> buffer(new char[buffer_size]);
  snprintf(buffer.get(), buffer_size, "%-20s %-5s %-7s %-8s %s",
           "NAME", "TYPE", "VALUE", "" ,"BLOCK");
  file->mutable_lines()->emplace_back(string(buffer.get()));
  for (const auto& entry_pair : entry_map_) {
    const string& name = entry_pair.first;
    const Entry& entry = entry_pair.second;
    const char* type_str = nullptr;
    if (entry.type == UNKNOWN && (entry.type == INTERNAL && !entry.defined)) {
      type_str = "U";
    } else if (entry.type == EXTERNAL) {
      type_str = "E";
    } else if (entry.type == INTERNAL) {
      if (entry.exported) {
        type_str = "X";
      } else {
        type_str = entry.relative ? "IR" : "IA";
      }
    }
    if (entry.type == EXTERNAL || entry.type == UNKNOWN ||
        (entry.type == INTERNAL && !entry.defined)) {
      snprintf(buffer.get(), buffer_size, "%-20.20s %-5s", name.c_str(), type_str);
    } else {
      snprintf(buffer.get(), buffer_size, "%-20.20s %-5s %06x  %-8u",
               name.c_str(), type_str, entry.address, entry.address);
    }
    string line = string(buffer.get());
    if (entry.block >= 0) {
      if (block_table == nullptr) {
        snprintf(buffer.get(), buffer_size, " #%-11d  %u",
                 entry.block, entry.block_address);
      } else {
        snprintf(buffer.get(), buffer_size, " %-12.12s  %u",
                 block_table->GetBlock(entry.block)->name.c_str(),
                 entry.block_address);
      }
      line += string(buffer.get());
    }
    file->mutable_lines()->emplace_back(std::move(line));
  }
}

const SymbolTable::EntryMap& SymbolTable::entry_map() const {
  return entry_map_;
}

SymbolTable::EntryMap* SymbolTable::mutable_entry_map() {
  return &entry_map_;
}

}  // namespace assembler
}  // namespace sicxe
