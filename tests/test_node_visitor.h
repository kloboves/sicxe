#ifndef TEST_NODE_VISITOR_H
#define TEST_NODE_VISITOR_H

#include <string>
#include "assembler/code.h"
#include "assembler/node_visitor.h"
#include "common/macros.h"
#include "common/text_file.h"

namespace sicxe {
namespace tests {

class TestNodeVisitor : assembler::ConstNodeVisitor {
 public:
  DISALLOW_COPY_AND_MOVE(TestNodeVisitor);

  static void DumpCode(const assembler::Code& code, TextFile* text_file);
  static void DumpCodeAndTables(const assembler::Code& code, TextFile* text_file);

 private:
  TestNodeVisitor();
  ~TestNodeVisitor();

  void DumpCodeInternal(const assembler::Code& code, TextFile* text_file);

  virtual void VisitNode(const assembler::node::Empty* node);
  virtual void VisitNode(const assembler::node::InstructionF1* node);
  virtual void VisitNode(const assembler::node::InstructionF2* node);
  virtual void VisitNode(const assembler::node::InstructionFS34* node);
  virtual void VisitNode(const assembler::node::DirectiveStart* node);
  virtual void VisitNode(const assembler::node::DirectiveEnd* node);
  virtual void VisitNode(const assembler::node::DirectiveOrg* node);
  virtual void VisitNode(const assembler::node::DirectiveEqu* node);
  virtual void VisitNode(const assembler::node::DirectiveUse* node);
  virtual void VisitNode(const assembler::node::DirectiveLtorg* node);
  virtual void VisitNode(const assembler::node::DirectiveBase* node);
  virtual void VisitNode(const assembler::node::DirectiveNobase* node);
  virtual void VisitNode(const assembler::node::DirectiveExtdef* node);
  virtual void VisitNode(const assembler::node::DirectiveExtref* node);
  virtual void VisitNode(const assembler::node::DirectiveMemInit* node);
  virtual void VisitNode(const assembler::node::DirectiveMemReserve* node);
  virtual void VisitNode(const assembler::node::DirectiveInternalLiteral* node);

  void OutputDirectiveName(const char* name);
  void OutputLabel(const assembler::node::Node* node);
  void OutputExpression(const assembler::node::Node::TokenList& expr);
  void OutputSymbolList(const assembler::node::Node::TokenList& symbols);
  void OutputInstruction(const assembler::node::Instruction* node);

  std::string line_;
};

}  // namespace tests
}  // namespace sicxe

#endif  // TEST_NODE_VISITOR_H
