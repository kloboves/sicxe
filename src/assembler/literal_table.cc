#include "assembler/literal_table.h"

#include <assert.h>
#include <stdlib.h>
#include <utility>
#include "common/float_util.h"
#include "common/text_file.h"

using std::make_pair;
using std::string;
using std::unique_ptr;

namespace sicxe {
namespace assembler {

LiteralTable::Entry::Entry(int the_id, TypeId the_type)
  : id(the_id), type(the_type) {}

LiteralTable::LiteralTable() {}
LiteralTable::~LiteralTable() {}

string LiteralTable::LiteralSymbolName(int literal_id) {
  char buffer[20];
  snprintf(buffer, 20, "*%d", literal_id);
  return string(buffer);
}

const LiteralTable::Entry* LiteralTable::GetLiteral(int literal_id) const {
  if (literal_id < 0 || literal_id >= static_cast<int>(entries_.size())) {
    return nullptr;
  }
  return entries_[literal_id].get();
}

int LiteralTable::NewLiteralByte(uint8 value) {
  auto it = byte_map_.find(value);
  if (it != byte_map_.end()) {
    return it->second->id;
  }

  int id = entries_.size();
  entries_.emplace_back(new Entry(id, BYTE));
  Entry* entry = entries_.back().get();
  entry->value.byte = value;
  byte_map_.insert(make_pair(value, entry));
  return id;
}

int LiteralTable::NewLiteralByte(const std::string& data) {
  assert(data.size() <= 1);
  uint8 value = 0;
  if (!data.empty()) {
    value = static_cast<uint8>(data[0]);
  }
  return NewLiteralByte(value);
}

int LiteralTable::NewLiteralWord(uint32 value) {
  auto it = word_map_.find(value);
  if (it != word_map_.end()) {
    return it->second->id;
  }

  int id = entries_.size();
  entries_.emplace_back(new Entry(id, WORD));
  Entry* entry = entries_.back().get();
  entry->value.word = value;
  word_map_.insert(make_pair(value, entry));
  return id;
}

int LiteralTable::NewLiteralWord(const std::string& data) {
  assert(data.size() <= 3);
  uint32 value = 0;
  for (size_t i = 0; i < data.size(); i++) {
    value <<= 8;
    value |= static_cast<uint8>(data[i]);
  }
  return NewLiteralWord(value);
}

int LiteralTable::NewLiteralFloat(const std::string& data) {
  assert(data.size() <= 6);
  uint64 value = 0;
  for (size_t i = 0; i < data.size(); i++) {
    value <<= 8;
    value |= static_cast<uint8>(data[i]);
  }

  auto it = float_map_.find(value);
  if (it != float_map_.end()) {
    return it->second->id;
  }

  int id = entries_.size();
  entries_.emplace_back(new Entry(id, FLOAT));
  Entry* entry = entries_.back().get();
  entry->value.float_bin = value;
  float_map_.insert(make_pair(value, entry));
  return id;
}

void LiteralTable::ClearLookupTables() {
  byte_map_.clear();
  word_map_.clear();
  float_map_.clear();
}

void LiteralTable::OutputToTextFile(TextFile* file) const {
  size_t buffer_size = 200;
  unique_ptr<char[]> buffer(new char[buffer_size]);
  snprintf(buffer.get(), buffer_size, "%-20s %-5s %s", "LITERAL", "TYPE", "VALUE");
  file->mutable_lines()->emplace_back(string(buffer.get()));
  for (const auto& entry : entries_) {
    snprintf(buffer.get(), buffer_size, "*%-19d ", entry->id);
    string line(buffer.get());
    if (entry->type == BYTE) {
      snprintf(buffer.get(), buffer_size, "%-5s %02x%-14s %u", "B",
               static_cast<uint32>(entry->value.byte) & 0xff, "",
               static_cast<uint32>(entry->value.byte) & 0xff);
    } else if (entry->type == WORD) {
      snprintf(buffer.get(), buffer_size, "%-5s %06x%-10s %u", "W",
               entry->value.word, "", entry->value.word);
    } else if (entry->type == FLOAT) {
      uint8 float_data[6];
      uint64 float_bin = entry->value.float_bin;
      for (size_t i = 0; i < 6; i++) {
        float_data[5 - i] = float_bin & 0xff;
        float_bin >>= 8;
      }
      double value = FloatUtil::DecodeFloatData(float_data);
      snprintf(buffer.get(), buffer_size, "%-5s %012llx     %lg", "F",
               entry->value.float_bin, value);
    } else {
      assert(false);
    }
    line += string(buffer.get());
    file->mutable_lines()->emplace_back(std::move(line));
  }
}

const LiteralTable::EntryVector& LiteralTable::entries() const {
  return entries_;
}

}  // namespace assembler
}  // namespace sicxe
