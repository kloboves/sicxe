#ifndef COMMON_OBJECT_FILE_H
#define COMMON_OBJECT_FILE_H

#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class ObjectFile {
 public:
  DISALLOW_COPY_AND_MOVE(ObjectFile);

  enum LoadResult {
    OK = 0,
    OPEN_FAILED,
    INVALID_FORMAT
  };

  struct TextSection {
    uint32 address;
    uint8 size;
    std::unique_ptr<uint8[]> data;
  };

  struct SymbolExportSection {
    // pairs name:address
    std::vector<std::pair<std::string, uint32> > symbols;
  };

  struct SymbolImportSection {
    // symbol names
    std::vector<std::string> symbols;
  };

  struct RelocationSection {
    uint32 address;
    uint8 nibbles;
    bool type;  // true means section with symbol reference
    bool sign;  // true = pluse; false = minus
    std::string symbol_name;
  };

  typedef std::vector<std::unique_ptr<TextSection> > TextSectionVector;
  typedef std::vector<std::unique_ptr<SymbolExportSection> > SymbolExportSectionVector;
  typedef std::vector<std::unique_ptr<SymbolImportSection> > SymbolImportSectionVector;
  typedef std::vector<std::unique_ptr<RelocationSection> > RelocationSectionVector;

  static const int kMaxSymbolsPerExportSection;
  static const int kMaxSymbolsPerImportSection;
  static const int kMaxTextSectionSize;

  ObjectFile();
  ~ObjectFile();

  const std::string& file_name() const;
  const std::string& program_name() const;
  uint32 start_address() const;
  uint32 code_size() const;
  uint32 entry_point() const;
  const TextSectionVector& text_sections() const;
  const SymbolExportSectionVector& export_sections() const;
  const SymbolImportSectionVector& import_sections() const;
  const RelocationSectionVector& relocation_sections() const;

  void set_file_name(const std::string& name);
  void set_program_name(const std::string& name);
  void set_start_address(uint32 address);
  void set_code_size(uint32 size);
  void set_entry_point(uint32 address);
  TextSectionVector* mutable_text_sections();
  SymbolExportSectionVector* mutable_export_sections();
  SymbolImportSectionVector* mutable_import_sections();
  RelocationSectionVector* mutable_relocation_sections();

  void Clear();
  LoadResult LoadFile(const char* the_file_name);
  bool SaveFile(const char* the_file_name) const;

 private:
  bool ParseStartSection(const char* line_buffer, size_t line_length);
  bool ParseEndSection(const char* line_buffer, size_t line_length);
  bool ParseTextSection(const char* line_buffer, size_t line_length);
  bool ParseSymbolExportSection(const char* line_buffer, size_t line_length);
  bool ParseSymbolImportSection(const char* line_buffer, size_t line_length);
  bool ParseRelocationSection(const char* line_buffer, size_t line_length);

  std::string file_name_;
  std::string program_name_;
  uint32 start_address_;
  uint32 code_size_;
  uint32 entry_point_;
  TextSectionVector text_sections_;
  SymbolExportSectionVector export_sections_;
  SymbolImportSectionVector import_sections_;
  RelocationSectionVector relocation_sections_;
};

}  // namespace sicxe

#endif  // COMMON_OBJECT_FILE_H
