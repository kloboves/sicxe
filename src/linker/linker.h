#ifndef LINKER_LINKER_H
#define LINKER_LINKER_H

#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"
#include "common/object_file.h"
#include "common/types.h"
#include "linker/relocation_table.h"
#include "linker/symbol_table.h"

namespace sicxe {

class ErrorDB;

namespace linker {

class Linker {
 public:
  DISALLOW_COPY_AND_MOVE(Linker);

  struct Config {
    Config();

    bool compatibility_mode;
  };

  typedef std::vector<const ObjectFile*> ObjectFileVector;

  Linker(const Config* config);
  ~Linker();

  bool LinkFiles(bool partial_link, uint32 start_address,
                 const ObjectFileVector& files, ObjectFile* output_file,
                 ErrorDB* error_db);

 private:
  bool OrderInputFiles();
  bool BuildSymbolTable();
  bool BuildRelocationTable();
  void InvalidRelocationError(const ObjectFile& file);
  bool PatchTextSection(const ObjectFile& file, uint32 address, uint8 nibbles,
                        int32 adjustment, ObjectFile::TextSection* text_section);
  void WriteRelocationSimple(uint32 address, uint8 nibbles);
  void WriteRelocationSymbol(uint32 address, uint8 nibbles, bool sign,
                             const std::string& symbol_name);
  bool SolveSymbolRelocation(const ObjectFile& file, const RelocationTable::Entry& entry,
                             int32* result, bool* relative, bool* all_symbols_known);
  bool CopyAndPatchCode();
  void WriteOutputSymbols();

  const Config* config_;
  bool partial_link_;
  uint32 start_address_;
  const ObjectFileVector* files_;
  ObjectFile* output_file_;
  ErrorDB* error_db_;

  std::unique_ptr<uint32[]> files_start_address_;
  std::unique_ptr<int32[]> files_address_adjustment_;
  uint32 end_address_;
  std::unique_ptr<SymbolTable> symbol_table_;
  std::unique_ptr<RelocationTable> relocation_table_;
};

}  // namespace linker
}  // namespace sicxe

#endif  // LINKER_LINKER_H
