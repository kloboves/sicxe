#ifndef ASSEMBLER_CODE_GENERATOR_H
#define ASSEMBLER_CODE_GENERATOR_H

#include <memory>
#include <string>
#include <vector>
#include "assembler/node.h"
#include "assembler/node_visitor.h"
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class ErrorDB;
struct InstructionInstance;

namespace assembler {

class Code;
class BlockTable;
class LiteralTable;
class SymbolTable;

struct RelocationRecord {
  enum TypeId {
    SIMPLE = 0,
    SYMBOL,
  };

  RelocationRecord(uint32 addr, int n);
  RelocationRecord(uint32 addr, int n, bool sgn, const std::string& name);
  ~RelocationRecord();

  TypeId type;
  uint32 address;
  int nibbles;
  bool sign;
  const std::string symbol_name;
};

class CodeOutputWriter {
 public:
  virtual void Begin(const Code& code) = 0;
  virtual void WriteNode(const node::Node& node, uint32 address) = 0;
  virtual void WriteNode(const node::Node& node, uint32 address,
                         const std::string& data, bool can_split) = 0;
  virtual void WriteRelocationRecord(const RelocationRecord& record) = 0;
  virtual void End() = 0;
};

class CodeGenerator : public ConstNodeVisitor {
 public:
  DISALLOW_COPY_AND_MOVE(CodeGenerator);

  typedef std::vector<CodeOutputWriter*> OutputWriterVector;

  CodeGenerator();
  ~CodeGenerator();

  bool GenerateCode(const Code& code, OutputWriterVector* output_writers,
                    ErrorDB* error_db);

 private:
  virtual void VisitNode(const node::Empty* node);
  virtual void VisitNode(const node::InstructionF1* node);
  virtual void VisitNode(const node::InstructionF2* node);
  virtual void VisitNode(const node::InstructionFS34* node);
  virtual void VisitNode(const node::DirectiveStart* node);
  virtual void VisitNode(const node::DirectiveEnd* node);
  virtual void VisitNode(const node::DirectiveOrg* node);
  virtual void VisitNode(const node::DirectiveEqu* node);
  virtual void VisitNode(const node::DirectiveUse* node);
  virtual void VisitNode(const node::DirectiveLtorg* node);
  virtual void VisitNode(const node::DirectiveBase* node);
  virtual void VisitNode(const node::DirectiveNobase* node);
  virtual void VisitNode(const node::DirectiveExtdef* node);
  virtual void VisitNode(const node::DirectiveExtref* node);
  virtual void VisitNode(const node::DirectiveMemInit* node);
  virtual void VisitNode(const node::DirectiveMemReserve* node);
  virtual void VisitNode(const node::DirectiveInternalLiteral* node);

  void Begin();
  void WriteNode(const node::Node& node);
  void WriteNode(const node::Node& node, const std::string& data, bool can_split);
  void WriteRelocationRecord(const RelocationRecord& record);
  void End();

  bool TryPCRelativeAddressing(int32 value, bool relative, InstructionInstance* instance);
  bool TryBaseAddressing(int32 value, bool relative, InstructionInstance* instance);
  bool CheckExpressionLimits(int32 limit_min, int32 limit_max, int32 value,
                             const node::Node::TokenList& expression);

  const Code* code_;
  const SymbolTable* symbol_table_;
  const BlockTable* block_table_;
  const LiteralTable* literal_table_;
  OutputWriterVector* output_writers_;
  ErrorDB* error_db_;
  int current_block_;
  uint32 current_address_;
  bool base_enabled_;
  bool base_relative_;
  uint32 base_address_;
  bool success_;
  std::unique_ptr<uint32[]> block_address_table_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_CODE_GENERATOR_H
