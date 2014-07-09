#include "assembler/object_file_writer.h"

#include <string.h>
#include <algorithm>
#include "assembler/code.h"
#include "assembler/symbol_table.h"

using std::unique_ptr;

namespace sicxe {
namespace assembler {

ObjectFileWriter::ObjectFileWriter(ObjectFile* object_file)
    : code_(nullptr), object_file_(object_file),
      text_buffer_(new uint8[ObjectFile::kMaxTextSectionSize]), text_address_(0),
      text_position_(0), text_next_address_(0) {}

ObjectFileWriter::~ObjectFileWriter() {}

void ObjectFileWriter::Begin(const Code& code) {
  code_ = &code;
}

void ObjectFileWriter::WriteNode(const node::Node&, uint32) {}

void ObjectFileWriter::CommitTextSection() {
  if (text_position_ > 0) {
    unique_ptr<ObjectFile::TextSection> section(new ObjectFile::TextSection);
    section->address = text_address_;
    section->size = text_position_;
    section->data.reset(new uint8[text_position_]);
    memcpy(section->data.get(), text_buffer_.get(), text_position_);
    object_file_->mutable_text_sections()->emplace_back(std::move(section));
    text_position_ = 0;
    text_address_ = text_next_address_;
  }
}

void ObjectFileWriter::WriteNode(const node::Node&, uint32 address,
                                 const std::string& data, bool can_split) {
  if (text_next_address_ != address) {
    CommitTextSection();
    text_address_ = address;
    text_next_address_ = address;
  }
  if (!can_split) {
    int size = data.size();
    assert(size <= ObjectFile::kMaxTextSectionSize);
    if (size > (ObjectFile::kMaxTextSectionSize - text_position_)) {
      CommitTextSection();
    }
    memcpy(text_buffer_.get() + text_position_, data.data(), size);
    text_position_ += size;
    text_next_address_ += size;
  } else {
    int size = data.size();
    int pos = 0;
    while (size > 0) {
      if ((ObjectFile::kMaxTextSectionSize - text_position_) == 0) {
        CommitTextSection();
      }
      int copy_size = std::min(ObjectFile::kMaxTextSectionSize - text_position_, size);
      memcpy(text_buffer_.get() + text_position_, data.data() + pos, copy_size);
      text_position_ += copy_size;
      text_next_address_ += copy_size;
      pos += copy_size;
      size -= copy_size;
    }
  }
}

void ObjectFileWriter::WriteRelocationRecord(const RelocationRecord& record) {
  unique_ptr<ObjectFile::RelocationSection> section(new ObjectFile::RelocationSection);
  section->address = record.address;
  section->nibbles = record.nibbles;
  section->type = (record.type == RelocationRecord::SYMBOL);
  if (record.type == RelocationRecord::SYMBOL) {
    section->sign = record.sign;
    section->symbol_name = record.symbol_name;
  }
  object_file_->mutable_relocation_sections()->emplace_back(std::move(section));
}

void ObjectFileWriter::End() {
  CommitTextSection();
  object_file_->set_program_name(code_->program_name());
  object_file_->set_start_address(code_->start_address());
  object_file_->set_code_size(code_->end_address() - code_->start_address());
  object_file_->set_entry_point(code_->entry_point());

  unique_ptr<ObjectFile::SymbolImportSection> import_section;
  unique_ptr<ObjectFile::SymbolExportSection> export_section;
  for (const auto& entry_pair : code_->symbol_table()->entry_map()) {
    const SymbolTable::Entry& entry = entry_pair.second;
    if (entry.type == SymbolTable::EXTERNAL &&
        entry.referenced) {
      if (import_section.get() == nullptr) {
        import_section.reset(new ObjectFile::SymbolImportSection);
      }
      import_section->symbols.push_back(entry_pair.first);
      if (static_cast<int>(import_section->symbols.size()) >=
          ObjectFile::kMaxSymbolsPerImportSection) {
        object_file_->mutable_import_sections()->emplace_back(std::move(import_section));
      }
    } else if (entry.type == SymbolTable::INTERNAL && entry.exported) {
      if (export_section.get() == nullptr) {
        export_section.reset(new ObjectFile::SymbolExportSection);
      }
      export_section->symbols.push_back(make_pair(entry_pair.first, entry.address));
      if (static_cast<int>(export_section->symbols.size()) >=
          ObjectFile::kMaxSymbolsPerExportSection) {
        object_file_->mutable_export_sections()->emplace_back(std::move(export_section));
      }
    }
  }
  if (import_section.get() != nullptr) {
    object_file_->mutable_import_sections()->emplace_back(std::move(import_section));
  }
  if (export_section.get() != nullptr) {
    object_file_->mutable_export_sections()->emplace_back(std::move(export_section));
  }
}

}  // namespace assembler
}  // namespace sicxe
