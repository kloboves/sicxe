#include "linker/linker.h"

#include <assert.h>
#include <string.h>
#include "common/error_db.h"

using std::string;
using std::unique_ptr;

namespace sicxe {
namespace linker {

Linker::Config::Config() : compatibility_mode(false) {}

Linker::Linker(const Config* config) : config_(config) {}
Linker::~Linker() {}

bool Linker::LinkFiles(bool partial_link, uint32 start_address,
                       const ObjectFileVector& files, ObjectFile* output_file,
                       ErrorDB* error_db) {
  partial_link_ = partial_link;
  start_address_ = start_address;
  files_ = &files;
  output_file_ = output_file;
  error_db_ = error_db;
  error_db_->SetCurrentFile(nullptr);

  if (config_->compatibility_mode && partial_link) {
    error_db_->AddError(ErrorDB::ERROR,
                        "partial linking not supported in compatibility mode");
    return false;
  }

  const ObjectFile& first_file = *(*files_)[0];
  if (first_file.entry_point() < first_file.start_address() ||
      first_file.entry_point() >= (first_file.start_address() + first_file.code_size())) {
    string message = "invalid entry point in file '";
    message += first_file.file_name();
    message += "'";
    error_db_->AddError(ErrorDB::ERROR, message.c_str());
    return false;
  }

  files_start_address_.reset(new uint32[files_->size()]);
  files_address_adjustment_.reset(new int32[files_->size()]);
  if (!OrderInputFiles()) {
    return false;
  }

  output_file_->set_program_name((*files_)[0]->program_name());
  output_file_->set_start_address(start_address_);
  output_file_->set_code_size(end_address_ - start_address_);
  output_file_->set_entry_point(static_cast<int32>((*files_)[0]->entry_point()) +
                                files_address_adjustment_[0]);

  symbol_table_.reset(new SymbolTable);
  if (!BuildSymbolTable()) {
    return false;
  }
  relocation_table_.reset(new RelocationTable);
  if (!BuildRelocationTable()) {
    return false;
  }
  if (!CopyAndPatchCode()) {
    return false;
  }
  WriteOutputSymbols();
  return true;
}

bool Linker::OrderInputFiles() {
  uint32 address = start_address_;
  bool success = true;
  for (size_t i = 0; i < files_->size(); i++) {
    const ObjectFile& file = *(*files_)[i];
    if (config_->compatibility_mode && file.start_address() != 0) {
      string message = "start address not 0 in file '";
      message += file.file_name();
      message += "'";
      error_db_->AddError(ErrorDB::ERROR, message.c_str());
      success = false;
    }
    files_start_address_[i] = address;
    files_address_adjustment_[i] = static_cast<int32>(files_start_address_[i]) -
                                   static_cast<int32>(file.start_address());
    address += file.code_size();
    if (address > 0x100000) {
      error_db_->AddError(ErrorDB::ERROR, "address overflow");
      return false;
    }
  }
  end_address_ = address;
  return success;
}

bool Linker::BuildSymbolTable() {
  bool success = true;
  for (size_t i = 0; i < files_->size(); i++) {
    const ObjectFile& file = *(*files_)[i];
    uint32 file_end_address = file.start_address() + file.code_size();

    // undefined symbols
    for (const auto& section : file.import_sections()) {
      for (const auto& symbol : section->symbols) {
        SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(symbol);
        entry->files_referenced.push_back(&file);
      }
    }

    // defined symbols
    for (const auto& section : file.export_sections()) {
      for (const auto& symbol : section->symbols) {
        SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(symbol.first);
        if (entry->type == SymbolTable::DEFINED) {
          string message = "duplicate symbol '" + symbol.first;
          message += "' in file '";
          message += file.file_name();
          message += "', first defined in file '";
          message += entry->file_defined->file_name();
          message += "'";
          error_db_->AddError(ErrorDB::ERROR, message.c_str());
          success = false;
          continue;
        }
        if (symbol.second < file.start_address() ||
            symbol.second >= file_end_address) {
          string message = "symbol '" + symbol.first;
          message += "' in file '";
          message += file.file_name();
          message += "' has invalid address";
          error_db_->AddError(ErrorDB::ERROR, message.c_str());
          success = false;
          continue;
        }
        entry->type = SymbolTable::DEFINED;
        entry->file_defined = &file;
        entry->address = static_cast<int32>(symbol.second) + files_address_adjustment_[i];
      }
    }
  }

  if (!success) {
    return false;
  }

  if (!partial_link_) {
    for (const auto& entry_pair : symbol_table_->entry_map()) {
      const SymbolTable::Entry& entry = entry_pair.second;
      if (entry.type == SymbolTable::UNDEFINED) {
        string message = "undefined symbol '" + entry_pair.first;
        message += "' referenced from files: ";
        bool first = true;
        for (const ObjectFile* file : entry.files_referenced) {
          if (!first) {
            message += ", ";
          }
          first = false;
          message += "'";
          message += file->file_name();
          message += "'";
        }
        error_db_->AddError(ErrorDB::ERROR, message.c_str());
        success = false;
      }
    }
  }
  return success;
}

bool Linker::BuildRelocationTable() {
  for (size_t i = 0; i < files_->size(); i++) {
    const ObjectFile& file = *(*files_)[i];
    uint32 file_end_address = file.start_address() + file.code_size();

    for (const auto& section : file.relocation_sections()) {
      if (section->address < file.start_address() ||
          section->address >= file_end_address ||
          section->nibbles == 0 || section->nibbles > 10) {
        InvalidRelocationError(file);
        return false;
      }
      uint32 address = static_cast<int32>(section->address) +
                       files_address_adjustment_[i];
      bool success = true;
      if (!section->type) {
        success = relocation_table_->AddRelocationSimple(address, section->nibbles);
      } else {
        success = relocation_table_->AddRelocationSymbol(
            address, section->nibbles, section->sign, section->symbol_name);
      }
      if (!success) {
        InvalidRelocationError(file);
        return false;
      }
    }
  }
  return true;
}

void Linker::InvalidRelocationError(const ObjectFile& file) {
  string message = "invalid relocation in file '" + file.file_name() + "'";
  error_db_->AddError(ErrorDB::ERROR, message.c_str());
}

bool Linker::PatchTextSection(const ObjectFile& file, uint32 address, uint8 nibbles,
                              int32 adjustment, ObjectFile::TextSection* text_section) {
  assert(nibbles > 0 && nibbles <= 10);
  assert(address >= text_section->address);
  uint32 position = address - text_section->address;
  uint32 space_left = static_cast<uint32>(text_section->size) - position;
  const size_t size = (nibbles + 1) / 2;  // ceil divide by two
  const int64 limit = (1 << (nibbles * 4)) - 1;
  if (size > space_left) {
    InvalidRelocationError(file);
    return false;
  }
  uint8* data = text_section->data.get() + position;
  uint8 upper_half_byte = 0;
  int64 result = 0;
  size_t i = 0;
  if (nibbles % 2 == 1) {
    upper_half_byte = data[0] & 0xf0;
    i = 1;
    result = data[0] & 0xf;
  }
  for (; i < size; i++) {
    result <<= 8;
    result |= data[i];
  }
  result += adjustment;
  if (result < 0) {
    string message = "relocation underflow in file '" + file.file_name() + "'";
    error_db_->AddError(ErrorDB::ERROR, message.c_str());
    return false;
  } else if (result > limit) {
    string message = "relocation overflow in file '" + file.file_name() + "'";
    error_db_->AddError(ErrorDB::ERROR, message.c_str());
    return false;
  }
  for (int j = size - 1; j >= 0; j--) {
    data[j] = result & 0xff;
    result >>= 8;
  }
  if (nibbles % 2 == 1) {
    data[0] |= upper_half_byte;
  }
  return true;
}

void Linker::WriteRelocationSimple(uint32 address, uint8 nibbles) {
  unique_ptr<ObjectFile::RelocationSection> section(new ObjectFile::RelocationSection);
  section->address = address;
  section->nibbles = nibbles;
  section->type = false;
  output_file_->mutable_relocation_sections()->emplace_back(std::move(section));
}

void Linker::WriteRelocationSymbol(uint32 address, uint8 nibbles, bool sign,
                                   const std::string& symbol_name) {
  unique_ptr<ObjectFile::RelocationSection> section(new ObjectFile::RelocationSection);
  section->address = address;
  section->nibbles = nibbles;
  section->type = true;
  section->sign = sign;
  section->symbol_name = symbol_name;
  output_file_->mutable_relocation_sections()->emplace_back(std::move(section));
}

bool Linker::SolveSymbolRelocation(const ObjectFile& file,
                                   const RelocationTable::Entry& entry, int32* result,
                                   bool* relative, bool* all_symbols_known) {
  int32 value = 0;
  int relative_count = 0;
  bool all_known = true;
  for (const auto& symbol : entry.symbols) {
    const SymbolTable::Entry* symbol_entry = symbol_table_->Find(symbol.second);
    if (symbol_entry == nullptr) {
      InvalidRelocationError(file);
      return false;
    }
    if (symbol.first) {
      relative_count++;
    } else {
      relative_count--;
    }
    if (symbol_entry->type != SymbolTable::DEFINED) {
      all_known = false;
      continue;
    }
    if (symbol.first) {
      value += static_cast<int32>(symbol_entry->address);
    } else {
      value -= static_cast<int32>(symbol_entry->address);
    }
  }
  if (!config_->compatibility_mode) {  // check if relative/absolute only in normal mode
    if (relative_count == 0) {
      *relative = false;
    } else if (relative_count == 1) {
      *relative = true;
    } else {
      InvalidRelocationError(file);
      return false;
    }
  } else {
    // all symbols should be known because partial linking is not allowed in
    // compatibility mode
    assert(all_known);
  }
  *all_symbols_known = all_known;
  *result = value;
  return true;
}

bool Linker::CopyAndPatchCode() {
  bool success = true;
  for (size_t i = 0; i < files_->size(); i++) {
    const ObjectFile& file = *(*files_)[i];
    for (const auto& section : file.text_sections()) {
      unique_ptr<ObjectFile::TextSection> patched_section(new ObjectFile::TextSection);
      patched_section->address = static_cast<int32>(section->address) +
                                 files_address_adjustment_[i];
      patched_section->size = section->size;
      patched_section->data.reset(new uint8[patched_section->size]);
      memcpy(patched_section->data.get(), section->data.get(), patched_section->size);

      // go through relocations whose address is in this text section
      uint32 range_start = patched_section->address;
      uint32 range_end = patched_section->address + patched_section->size;
      auto it = relocation_table_->entry_map().lower_bound(range_start);
      for (; it != relocation_table_->entry_map().end() && it->first < range_end; ++it) {
        uint32 address = it->first;
        const RelocationTable::Entry& entry = it->second;
        int32 adjustment = 0;
        bool relative = false;
        bool all_symbols_known = false;

        if (entry.type == RelocationTable::SIMPLE) {
          adjustment = files_address_adjustment_[i];
        } else if (entry.type == RelocationTable::SYMBOL) {
          if (!SolveSymbolRelocation(file, entry, &adjustment, &relative,
                                     &all_symbols_known)) {
            success = false;
            break;
          }
        }

        if (config_->compatibility_mode || entry.type == RelocationTable::SIMPLE ||
            (entry.type == RelocationTable::SYMBOL && all_symbols_known)) {
          if (!PatchTextSection(file, address, entry.nibbles, adjustment,
                                patched_section.get())) {
            success = false;
            continue;
          }
        }

        if (!config_->compatibility_mode) {
          // write output relocation records (only in normal mode)
          if (entry.type == RelocationTable::SIMPLE) {
            WriteRelocationSimple(address, entry.nibbles);
          } else if (entry.type == RelocationTable::SYMBOL) {
            if (!all_symbols_known) {
              for (const auto& symbol : entry.symbols) {
                WriteRelocationSymbol(address, entry.nibbles, symbol.first,
                                      symbol.second);
              }
            } else if (relative) {
              WriteRelocationSimple(address, entry.nibbles);
            }
          }
        }
      }
      output_file_->mutable_text_sections()->emplace_back(std::move(patched_section));
    }
  }
  return success;
}

void Linker::WriteOutputSymbols() {
  unique_ptr<ObjectFile::SymbolImportSection> import_section;
  unique_ptr<ObjectFile::SymbolExportSection> export_section;
  for (const auto& entry_pair : symbol_table_->entry_map()) {
    const SymbolTable::Entry& entry = entry_pair.second;
    if (entry.type == SymbolTable::UNDEFINED) {
      if (import_section.get() == nullptr) {
        import_section.reset(new ObjectFile::SymbolImportSection);
      }
      import_section->symbols.push_back(entry_pair.first);
      if (static_cast<int>(import_section->symbols.size()) >=
          ObjectFile::kMaxSymbolsPerImportSection) {
        output_file_->mutable_import_sections()->emplace_back(std::move(import_section));
      }
    } else if (entry.type == SymbolTable::DEFINED) {
      if (export_section.get() == nullptr) {
        export_section.reset(new ObjectFile::SymbolExportSection);
      }
      export_section->symbols.push_back(make_pair(entry_pair.first,
                                                  entry.address));
      if (static_cast<int>(export_section->symbols.size()) >=
          ObjectFile::kMaxSymbolsPerExportSection) {
        output_file_->mutable_export_sections()->emplace_back(std::move(export_section));
      }
    }
  }
  if (import_section.get() != nullptr) {
    output_file_->mutable_import_sections()->emplace_back(std::move(import_section));
  }
  if (export_section.get() != nullptr) {
    output_file_->mutable_export_sections()->emplace_back(std::move(export_section));
  }
}

}  // namespace linker
}  // namespace sicxe
