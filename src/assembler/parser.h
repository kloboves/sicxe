#ifndef ASSEMBLER_PARSER_H
#define ASSEMBLER_PARSER_H

#include <list>
#include <memory>
#include <string>
#include "assembler/node.h"
#include "assembler/node_visitor.h"
#include "assembler/token.h"
#include "common/macros.h"

namespace sicxe {

class ErrorDB;
class InstructionDB;
class TextFile;

namespace assembler {

class Code;

class Parser : public NodeVisitor {
 public:
  DISALLOW_COPY_AND_MOVE(Parser);

  struct Config {
    Config();

    const InstructionDB* instruction_db;
    bool case_sensitive;  // directives and mnemonics are case sensitive
    bool allow_brackets;
  };

  typedef node::Node::TokenList TokenList;

  explicit Parser(const Config* config);
  ~Parser();

  bool ParseFile(const TextFile& file, Code* code, ErrorDB* error_db);

 private:
  bool ParseLine();

  bool ParseRegisterName(const Token& token, uint8* register_id);
  bool ParseExpression(bool allow_brackets, TokenList* expression);
  bool ParseAddressingOperator(node::InstructionFS34* node);
  bool CheckDataTokenMaxSize(const Token& data_token, Syntax::SyntaxId syntax);
  bool ParseIndexOperator(node::InstructionFS34* node);
  void VisitExpressionDirective(node::ExpressionDirective* node);
  void VisitSymbolListDirective(node::SymbolListDirective* node);

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

  void EndOfLineError(const Token& last_token);
  void LabelNotAllowedError(const Token& label);

  const Config* config_;
  Code* code_;
  ErrorDB* error_db_;
  TokenList tokens_;
  std::unique_ptr<node::Node> node_;
  bool extended_;
  bool visit_success_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_PARSER_H
