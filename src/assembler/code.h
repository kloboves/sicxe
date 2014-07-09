#ifndef ASSEMBLER_CODE_H
#define ASSEMBLER_CODE_H

#include <list>
#include <memory>
#include <string>
#include "assembler/node.h"
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class TextFile;

namespace assembler {

class BlockTable;
class LiteralTable;
class SymbolTable;

class Code {
 public:
  DISALLOW_COPY_AND_MOVE(Code);

  typedef std::list<std::unique_ptr<node::Node> > NodeList;

  Code();
  ~Code();

  const TextFile* text_file() const;
  void set_text_file(const TextFile* file);
  const NodeList& nodes() const;
  NodeList* mutable_nodes();
  const SymbolTable* symbol_table() const;
  void set_symbol_table(SymbolTable* table);
  const BlockTable* block_table() const;
  void set_block_table(BlockTable* table);
  const LiteralTable* literal_table() const;
  void set_literal_table(LiteralTable* table);
  const std::string& program_name() const;
  void set_program_name(const std::string& name);
  uint32 start_address() const;
  void set_start_address(uint32 the_start_address);
  uint32 end_address() const;
  void set_end_address(uint32 the_end_address);
  uint32 entry_point() const;
  void set_entry_point(uint32 the_entry_point);

 private:
  const TextFile* text_file_;
  NodeList nodes_;
  std::unique_ptr<SymbolTable> symbol_table_;
  std::unique_ptr<BlockTable> block_table_;
  std::unique_ptr<LiteralTable> literal_table_;
  std::string program_name_;
  uint32 start_address_;  // lowest address of code
  uint32 end_address_;  // highes address (address of first free byte)
  uint32 entry_point_;  // address where program execution starts
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_CODE_H
