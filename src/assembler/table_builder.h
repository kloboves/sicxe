#ifndef ASSEMBLER_TABLE_BUILDER_H
#define ASSEMBLER_TABLE_BUILDER_H

#include <list>
#include <utility>
#include <vector>
#include "assembler/code.h"
#include "assembler/node.h"
#include "assembler/node_visitor.h"
#include "assembler/symbol_table.h"
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class ErrorDB;

namespace assembler {

class BlockTable;
class LiteralTable;

class TableBuilder : public NodeVisitor {
 public:
  DISALLOW_COPY_AND_MOVE(TableBuilder);

  TableBuilder();
  ~TableBuilder();

  bool BuildTables(Code* code, ErrorDB* error_db);

 private:
  virtual void VisitNode(node::Empty* node);
  virtual void VisitNode(node::InstructionF1* node);
  virtual void VisitNode(node::InstructionF2* node);
  virtual void VisitNode(node::InstructionFS34* node);
  virtual void VisitNode(node::DirectiveStart* node);
  virtual void VisitNode(node::DirectiveEnd* node);
  virtual void VisitNode(node::DirectiveOrg* node);
  virtual void VisitNode(node::DirectiveEqu* node);
  virtual void VisitNode(node::DirectiveUse* node);
  virtual void VisitNode(node::DirectiveLtorg* node);
  virtual void VisitNode(node::DirectiveBase* node);
  virtual void VisitNode(node::DirectiveNobase* node);
  virtual void VisitNode(node::DirectiveExtdef* node);
  virtual void VisitNode(node::DirectiveExtref* node);
  virtual void VisitNode(node::DirectiveMemInit* node);
  virtual void VisitNode(node::DirectiveMemReserve* node);
  virtual void VisitNode(node::DirectiveInternalLiteral* node);

  void InsertLiterals(bool after_node);
  bool SolveAbsoluteExpression(const node::Node::TokenList& expression,
                               int32 limit_min, int32 limit_max, int32* result);
  bool SolveRelativeExpression(const node::Node::TokenList& expression,
                               int32 limit_min, int32 limit_max, int32* result);
  void EndSegment();
  bool CheckAddressOverflow(uint32 address);
  bool CheckExpressionLimits(int32 limit_min, int32 limit_max, int32 value,
                             const node::Node::TokenList& expression);
  bool DefineSymbol(const Token& token);
  void ReferenceSymbol(const std::string& name);
  // If the symbol is unknown also adds the token to |unknown_symbol_tokens_|
  void ReferenceSymbolToken(const Token& token);
  void ReferenceExpression(const node::Node::TokenList& expression);
  bool ImportSymbol(const Token& token);
  bool ExportSymbol(const Token& token);
  bool OrderBlocks();
  bool CheckIntersect();
  void FinalizeSymbolTable(bool* has_undefined_symbols);
  void UndefinedSymbolsError();
  uint32 GetEndAddress();

  Code* code_;
  std::unique_ptr<SymbolTable> symbol_table_;
  std::unique_ptr<BlockTable> block_table_;
  std::unique_ptr<LiteralTable> literal_table_;
  ErrorDB* error_db_;

  uint32 start_address_;
  uint32 entry_point_;
  std::vector<std::pair<uint32, uint32> > segments_;  // for checking intersections
  std::list<std::pair<const Token*, const SymbolTable::Entry*> > undefined_symbol_tokens_;
  uint32 segment_start_;
  int current_block_;
  uint32 current_address_;
  bool success_;
  int next_literal_id_;  // next literal that has not yet been inserted
  Code::NodeList::iterator it_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_TABLE_BUILDER_H
