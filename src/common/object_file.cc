#include "common/object_file.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

using std::pair;
using std::string;
using std::unique_ptr;

namespace sicxe {

const int ObjectFile::kMaxSymbolsPerExportSection = 6;
const int ObjectFile::kMaxSymbolsPerImportSection = 12;
const int ObjectFile::kMaxTextSectionSize = 30;

ObjectFile::ObjectFile() : start_address_(0), code_size_(0), entry_point_(0) {}

ObjectFile::~ObjectFile() {}

const string& ObjectFile::file_name() const {
  return file_name_;
}

const string& ObjectFile::program_name() const {
  return program_name_;
}

uint32 ObjectFile::start_address() const {
  return start_address_;
}

uint32 ObjectFile::code_size() const {
  return code_size_;
}

uint32 ObjectFile::entry_point() const {
  return entry_point_;
}

const ObjectFile::TextSectionVector& ObjectFile::text_sections() const {
  return text_sections_;
}

const ObjectFile::SymbolExportSectionVector& ObjectFile::export_sections() const {
  return export_sections_;
}

const ObjectFile::SymbolImportSectionVector& ObjectFile::import_sections() const {
  return import_sections_;
}

const ObjectFile::RelocationSectionVector& ObjectFile::relocation_sections() const {
  return relocation_sections_;
}

void ObjectFile::set_file_name(const string& name) {
  file_name_ = name;
}

void ObjectFile::set_program_name(const string& name) {
  program_name_ = name;
}

void ObjectFile::set_start_address(uint32 address) {
  start_address_ = address;
}

void ObjectFile::set_code_size(uint32 size) {
  code_size_ = size;
}

void ObjectFile::set_entry_point(uint32 address) {
  entry_point_ = address;
}

ObjectFile::TextSectionVector* ObjectFile::mutable_text_sections() {
  return &text_sections_;
}

ObjectFile::SymbolExportSectionVector* ObjectFile::mutable_export_sections() {
  return &export_sections_;
}

ObjectFile::SymbolImportSectionVector* ObjectFile::mutable_import_sections() {
  return &import_sections_;
}

ObjectFile::RelocationSectionVector* ObjectFile::mutable_relocation_sections() {
  return &relocation_sections_;
}

void ObjectFile::Clear() {
  file_name_.clear();
  program_name_.clear();
  start_address_ = 0;
  code_size_ = 0;
  entry_point_ = 0;
  text_sections_.clear();
  export_sections_.clear();
  import_sections_.clear();
  relocation_sections_.clear();
}

namespace {

bool ParseHex(const char* buffer, int length, uint32* result) {
  uint32 rv = 0;
  for (int i = 0; i < length; i++) {
    uint32 c = 0;
    if (buffer[i] >= '0' && buffer[i] <= '9') {
      c = buffer[i] - '0';
    } else if (buffer[i] >= 'A' && buffer[i] <= 'F') {
      c = (buffer[i] - 'A') + 10;
    } else if (buffer[i] >= 'a' && buffer[i] <= 'f') {
      c = (buffer[i] - 'a') + 10;
    }
    else {
      return false;
    }
    rv <<= 4;
    rv |= c;
  }
  *result = rv;
  return true;
}

bool ParseSymbolName(const char* buffer, int length, string* result) {
  string rv;
  rv.resize(length, ' ');
  for (int i = 0; i < length; i++) {
    if (isalnum(buffer[i]) || buffer[i] == ' ' || buffer[i] == '-' ||
        buffer[i] == '_' || buffer[i] == '+') {
      rv[i] = buffer[i];
    } else {
      return false;
    }
  }
  size_t pos = rv.find_last_not_of(' ');
  if (pos == string::npos) {
    return false;
  }
  rv.resize(pos + 1);
  *result = rv;
  return true;
}

}  // namespace

bool ObjectFile::ParseStartSection(const char* line_buffer, size_t line_length) {
  return (line_length == 19 &&
          ParseSymbolName(line_buffer + 1, 6, &program_name_) &&
          ParseHex(line_buffer + 7, 6, &start_address_) &&
          ParseHex(line_buffer + 13, 6, &code_size_));
}

bool ObjectFile::ParseEndSection(const char* line_buffer, size_t line_length) {
  return (line_length == 7 &&
          ParseHex(line_buffer + 1, 6, &entry_point_));
}

bool ObjectFile::ParseTextSection(const char* line_buffer, size_t line_length) {
  if (line_length < 9) {
    return false;
  }
  unique_ptr<TextSection> section(new TextSection);
  uint32 size = 0;
  if (!ParseHex(line_buffer + 1, 6, &section->address) ||
      !ParseHex(line_buffer + 7, 2, &size)) {
    return false;
  }
  if (size > 31 || (2 * size) + 9 != line_length) {
    return false;
  }
  section->size = size;
  section->data.reset(new uint8[size]);
  for (size_t i = 0; i < size; i++) {
    uint32 byte = 0;
    if (!ParseHex(line_buffer + 9 + 2 * i, 2, &byte)) {
      return false;
    }
    section->data[i] = byte;
  }
  text_sections_.emplace_back(std::move(section));
  return true;
}

bool ObjectFile::ParseSymbolExportSection(const char* line_buffer, size_t line_length) {
  if ((line_length - 1) % 12 != 0) {
    return false;
  }
  unique_ptr<SymbolExportSection> section(new SymbolExportSection);
  int num_symbols = (line_length - 1) / 12;
  for (int i = 0; i < num_symbols; i++) {
    string name;
    uint32 address = 0;
    if (!ParseSymbolName(line_buffer + 12 * i + 1, 6, &name) ||
        !ParseHex(line_buffer + 12 * i + 7, 6, &address)) {
      return false;
    }
    section->symbols.push_back(pair<string, uint32>(name, address));
  }
  export_sections_.emplace_back(std::move(section));
  return true;
}

bool ObjectFile::ParseSymbolImportSection(const char* line_buffer, size_t line_length) {
  if ((line_length - 1) % 6 != 0) {
    return false;
  }
  unique_ptr<SymbolImportSection> section(new SymbolImportSection);
  int num_symbols = (line_length - 1) / 6;
  for (int i = 0; i < num_symbols; i++) {
    string name;
    if (!ParseSymbolName(line_buffer + 6 * i + 1, 6, &name)) {
      return false;
    }
    section->symbols.push_back(name);
  }
  import_sections_.emplace_back(std::move(section));
  return true;
}

bool ObjectFile::ParseRelocationSection(const char* line_buffer, size_t line_length) {
  if (line_length != 9 && line_length != 16) {
    return false;
  }
  unique_ptr<RelocationSection> section(new RelocationSection);
  uint32 nibbles;
  if (!ParseHex(line_buffer + 1, 6, &section->address) ||
      !ParseHex(line_buffer + 7, 2, &nibbles)) {
    return false;
  }
  section->nibbles = static_cast<uint8>(nibbles);
  if (line_length == 9) {
    section->type = false;
  } else {
    section->type = true;
    if (line_buffer[9] == '+') {
      section->sign = true;
    } else if (line_buffer[9] == '-') {
      section->sign = false;
    } else {
      return false;
    }
    if (!ParseSymbolName(line_buffer + 10, 6, &section->symbol_name)) {
      return false;
    }
  }
  relocation_sections_.emplace_back(std::move(section));
  return true;
}

ObjectFile::LoadResult ObjectFile::LoadFile(const char* the_file_name) {
  FILE* file = fopen(the_file_name, "r");
  if (file == nullptr) {
    return OPEN_FAILED;
  }
  Clear();
  file_name_ = string(the_file_name);

  bool success = true;
  int start_section_position = -1;
  int end_section_position = -1;

  // read lines
  unique_ptr<char[]> line_buffer(new char[1024]);
  size_t line_length = 0;
  int line = 0;
  for (line = 0; true; line++) {
    if (fgets(line_buffer.get(), 1024, file) == nullptr) {
      break;
    }
    line_length = strlen(line_buffer.get());
    if (line_buffer[line_length - 1] == '\n') {
      line_length--;
      line_buffer[line_length] = '\0';
    }

    char section_type = line_buffer[0];

    if (section_type == 'H') {
      if (start_section_position != -1 ||
          !ParseStartSection(line_buffer.get(), line_length)) {
        success = false;
        break;
      }
      start_section_position = line;
    } else if (section_type == 'E') {
      if (end_section_position != -1 ||
          !ParseEndSection(line_buffer.get(), line_length)) {
        success = false;
        break;
      }
      end_section_position = line;
    } else if (section_type == 'T') {
      if (!ParseTextSection(line_buffer.get(), line_length)) {
        success = false;
        break;
      }
    } else if (section_type == 'D') {
      if (!ParseSymbolExportSection(line_buffer.get(), line_length)) {
        success = false;
        break;
      }
    } else if (section_type == 'R') {
      if (!ParseSymbolImportSection(line_buffer.get(), line_length)) {
        success = false;
        break;
      }
    } else if (section_type == 'M') {
      if (!ParseRelocationSection(line_buffer.get(), line_length)) {
        success = false;
        break;
      }
    } else {
      success = false;
      break;
    }
  }

  if (start_section_position != 0 || end_section_position != line - 1) {
    success = false;
  }
  fclose(file);

  if (success) {
    return OK;
  } else {
    Clear();
    return INVALID_FORMAT;
  }
}

bool ObjectFile::SaveFile(const char* the_file_name) const {
  FILE* file = fopen(the_file_name, "w");
  if (file == nullptr) {
    return false;
  }
  // start section
  assert(program_name_.size() <= 6);
  fprintf(file, "H%-6.6s%06X%06X\n", program_name_.c_str(),
                                     start_address_ & 0xfffff,
                                     code_size_ & 0xfffff);
  // text sections
  for (const auto& section : text_sections_) {
    int size = static_cast<uint32>(section->size) & 0xff;
    fprintf(file, "T%06X%02X", section->address & 0xffffff, size);
    for (int i = 0; i < size; i++) {
      uint32 byte = section->data[i];
      fprintf(file, "%02X", byte);
    }
    fprintf(file, "\n");
  }
  // export symbols
  for (const auto& section : export_sections_) {
    fprintf(file, "D");
    for (const auto& symbol : section->symbols) {
      assert(symbol.first.size() <= 6);
      fprintf(file, "%-6.6s%06X", symbol.first.c_str(),
                                  symbol.second & 0xfffff);
    }
    fprintf(file, "\n");
  }
  // import symbols
  for (const auto& section : import_sections_) {
    fprintf(file, "R");
    for (const auto& symbol : section->symbols) {
      assert(symbol.size() <= 6);
      fprintf(file, "%-6.6s", symbol.c_str());
    }
    fprintf(file, "\n");
  }
  // relocation sections
  for (const auto& section : relocation_sections_) {
    fprintf(file, "M");
    fprintf(file, "%06X%02X", section->address,
                              static_cast<uint32>(section->nibbles) & 0xff);
    if (section->type) {
      assert(section->symbol_name.size() <= 6);
      fprintf(file, "%c%-6.6s", section->sign ? '+' : '-',
                                section->symbol_name.c_str());
    }
    fprintf(file, "\n");
  }
  // end section
  fprintf(file, "E%06X\n", entry_point_ & 0xfffff);
  fclose(file);
  return true;
}

}  // namespace sicxe
