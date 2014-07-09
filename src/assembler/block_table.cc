#include "assembler/block_table.h"

#include "common/text_file.h"

using std::map;
using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {
namespace assembler {

BlockTable::Entry::Entry(int the_block, const string& the_name)
  : block(the_block), name(the_name), start(0), size(0) {}

BlockTable::BlockTable() {
  entries_.push_back(unique_ptr<Entry>(new Entry(0, "<default>")));
}

BlockTable::~BlockTable() {}

const BlockTable::Entry* BlockTable::GetBlock(int block) const {
  if (block < static_cast<int>(entries_.size())) {
    return entries_[block].get();
  }
  return nullptr;
}

BlockTable::Entry* BlockTable::GetBlock(int block) {
  if (block < static_cast<int>(entries_.size())) {
    return entries_[block].get();
  }
  return nullptr;
}

const BlockTable::Entry* BlockTable::FindName(const string& name) const {
  auto it = name_map_.find(name);
  if (it == name_map_.end()) {
    return nullptr;
  }
  return it->second;
}

BlockTable::Entry* BlockTable::FindNameOrCreate(const string& name) {
  auto p = name_map_.insert(make_pair(name, nullptr));
  if (p.second) {
    int block = entries_.size();
    entries_.emplace_back(new Entry(block, name));
    return (p.first->second = entries_.back().get());
  } else {
    return p.first->second;
  }
}

void BlockTable::OrderBlocks(uint32* start, uint32* end) {
  uint32 address = entries_[0]->start;
  *start = address;
  for (auto& entry : entries_) {
    entry->start = address;
    address += entry->size;
  }
  *end = address;
}

void BlockTable::OutputToTextFile(TextFile* file) const {
  size_t buffer_size = 200;
  unique_ptr<char[]> buffer(new char[buffer_size]);
  snprintf(buffer.get(), buffer_size, "%-26s %-7s %-8s %s",
           "NAME", "START", "", "SIZE");
  file->mutable_lines()->emplace_back(string(buffer.get()));
  for (const auto& entry : entries_) {
    snprintf(buffer.get(), buffer_size, "%-26.26s %06x  %-8u %u",
             entry->name.c_str(), entry->start, entry->start, entry->size);
    file->mutable_lines()->emplace_back(string(buffer.get()));
  }
}

const BlockTable::EntryVector& BlockTable::entries() const {
  return entries_;
}

}  // namespace assembler
}  // namespace sicxe
