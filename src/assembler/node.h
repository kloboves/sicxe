#ifndef ASSEMBLER_NODE_H
#define ASSEMBLER_NODE_H

#include <list>
#include <memory>
#include "assembler/directive.h"
#include "assembler/token.h"
#include "common/format.h"
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {
namespace assembler {

class NodeVisitor;
class ConstNodeVisitor;

namespace node {

// Hierarchy of assembler node classes:
//
//  (-) Node
//   |  (+) Empty
//   |  (-) Instruction
//   |   |  (+) InstructionF1
//   |   |  (+) InstructionF2
//   |   |  (+) InstructionFS34
//   |  (-) Directive
//   |   |  (-) ExpressionDirective
//   |   |   |  (+) DirectiveStart
//   |   |   |  (+) DirectiveEnd
//   |   |   |  (+) DirectiveOrg
//   |   |   |  (+) DirectiveEqu
//   |   |   |  (+) DirectiveBase
//   |   |   |  (+) DirectiveMemInit
//   |   |   |  (+) DirectiveMemReserve
//   |   |  (-) SymbolListDirective
//   |   |   |  (+) DirectiveExtdef
//   |   |   |  (+) DirectiveExtref
//   |   |  (+) DirectiveUse
//   |   |  (+) DirectiveLtorg
//   |   |  (+) DirectiveNobase
//   |   |  (+) DirectiveInternalLiteral


// Root node
class Node {
 public:
  DISALLOW_COPY_AND_MOVE(Node);

  enum NodeKind {
    NK_Node = 0,
    NK_Empty,
    NK_Instruction,
    NK_InstructionF1,
    NK_InstructionF2,
    NK_InstructionFS34,
    NK_Instruction_Last,
    NK_Directive,
    NK_ExpressionDirective,
    NK_DirectiveStart,
    NK_DirectiveEnd,
    NK_DirectiveOrg,
    NK_DirectiveEqu,
    NK_DirectiveBase,
    NK_DirectiveMemInit,
    NK_DirectiveMemReserve,
    NK_ExpressionDirective_Last,
    NK_SymbolListDirective,
    NK_DirectiveExtdef,
    NK_DirectiveExtref,
    NK_SymbolListDirective_Last,
    NK_DirectiveUse,
    NK_DirectiveLtorg,
    NK_DirectiveNobase,
    NK_DirectiveInternalLiteral,
    NK_Directive_Last,
    NK_Node_Last
  };

  typedef std::list<std::unique_ptr<Token> > TokenList;

  Node(NodeKind the_kind);
  virtual ~Node();

  virtual void AcceptVisitor(NodeVisitor* visitor) = 0;
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const = 0;

  NodeKind kind() const;
  const Token* label() const;
  void set_label(Token* the_label);
  const Token* comment() const;
  void set_comment(Token* the_comment);

 private:
  const NodeKind kind_;
  std::unique_ptr<Token> label_;
  std::unique_ptr<Token> comment_;
};

// Empty node (blank line)
class Empty : public Node {
 public:
  Empty();
  virtual ~Empty();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

// Abstract instruction node
class Instruction : public Node {
 public:
  Instruction(NodeKind the_kind, Format::FormatId the_format);
  virtual ~Instruction();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor) = 0;
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const = 0;

  Format::FormatId format() const;
  uint8 opcode() const;
  void set_opcode(uint8 the_opcode);
  Syntax::SyntaxId syntax() const;
  void set_syntax(Syntax::SyntaxId id);
  const Token* mnemonic() const;
  void set_mnemonic(Token* the_mnemonic);

 private:
  const Format::FormatId format_;
  uint8 opcode_;
  Syntax::SyntaxId syntax_;
  std::unique_ptr<Token> mnemonic_;
};

// Instruction nodes
class InstructionF1 : public Instruction {
 public:
  InstructionF1();
  virtual ~InstructionF1();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

class InstructionF2 : public Instruction {
 public:
  InstructionF2();
  virtual ~InstructionF2();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;

  uint8 r1() const;
  void set_r1(uint8 value);
  uint8 r2() const;
  void set_r2(uint8 value);

 private:
  uint8 r1_;
  uint8 r2_;
};

class InstructionFS34 : public Instruction {
 public:
  enum AddressingId {
    SIMPLE = 0,
    IMMEDIATE,
    INDIRECT,
    LITERAL_POOL
  };

  InstructionFS34();
  virtual ~InstructionFS34();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;

  const TokenList& expression() const;
  TokenList* mutable_expression();
  const Token* data_token() const;
  void set_data_token(Token* the_data_token);
  AddressingId addressing() const;
  void set_addressing(AddressingId the_addressing);
  bool extended() const;
  void set_extended(bool value);
  bool indexed() const;
  void set_indexed(bool value);
  int literal_id() const;
  void set_literal_id(int the_literal_id);

 private:
  TokenList expression_;
  std::unique_ptr<Token> data_token_;
  AddressingId addressing_;
  bool extended_;
  bool indexed_;
  int literal_id_;
};

// Abstract directive node
class Directive : public Node {
 public:
  typedef assembler::Directive::DirectiveId DirectiveId;

  Directive(NodeKind the_kind, DirectiveId the_id);
  virtual ~Directive();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor) = 0;
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const = 0;

  DirectiveId directive_id() const;
  const Token* mnemonic() const;
  void set_mnemonic(Token* the_mnemonic);

 private:
  const DirectiveId directive_id_;
  std::unique_ptr<Token> mnemonic_;
};

// Abstract nodes for types of similar directives
class ExpressionDirective : public Directive {
 public:
  ExpressionDirective(NodeKind the_kind, assembler::Directive::DirectiveId dr_id);
  virtual ~ExpressionDirective();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor) = 0;
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const = 0;

  const TokenList& expression() const;
  TokenList* mutable_expression();

 private:
  TokenList expression_;
};

class SymbolListDirective : public Directive {
 public:
  SymbolListDirective(NodeKind the_kind, assembler::Directive::DirectiveId dr_id);
  virtual ~SymbolListDirective();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor) = 0;
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const = 0;

  const TokenList& symbol_list() const;
  TokenList* mutable_symbol_list();

 private:
  TokenList symbol_list_;
};

// Directive nodes
class DirectiveStart : public ExpressionDirective {
 public:
  DirectiveStart();
  virtual ~DirectiveStart();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

class DirectiveEnd : public ExpressionDirective {
 public:
  DirectiveEnd();
  virtual ~DirectiveEnd();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

class DirectiveOrg : public ExpressionDirective {
 public:
  DirectiveOrg();
  virtual ~DirectiveOrg();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;

  uint32 address() const;
  void set_address(uint32 the_address);

 private:
  uint32 address_;
};

class DirectiveEqu : public ExpressionDirective {
 public:
  DirectiveEqu();
  virtual ~DirectiveEqu();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;

  bool assign_current_address() const;
  void set_assign_current_address(bool value);

 private:
  bool assign_current_address_;
};

class DirectiveUse : public Directive {
 public:
  DirectiveUse();
  virtual ~DirectiveUse();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;

  int block_id() const;
  void set_block_id(int id);
  const Token* block_name() const;
  void set_block_name(Token* the_block_name);

 private:
  int block_id_;
  std::unique_ptr<Token> block_name_;
};

class DirectiveLtorg : public Directive {
 public:
  DirectiveLtorg();
  virtual ~DirectiveLtorg();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

class DirectiveBase : public ExpressionDirective {
 public:
  DirectiveBase();
  virtual ~DirectiveBase();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

class DirectiveNobase : public Directive {
 public:
  DirectiveNobase();
  virtual ~DirectiveNobase();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

class DirectiveExtdef : public SymbolListDirective {
 public:
  DirectiveExtdef();
  virtual ~DirectiveExtdef();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

class DirectiveExtref : public SymbolListDirective {
 public:
  DirectiveExtref();
  virtual ~DirectiveExtref();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;
};

// directives BYTE and WORD
class DirectiveMemInit : public ExpressionDirective {
 public:
  enum WidthId {
    BYTE = 0,
    WORD
  };

  explicit DirectiveMemInit(WidthId the_width);
  virtual ~DirectiveMemInit();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;

  WidthId width() const;
  const Token* data_token() const;
  void set_data_token(Token* the_data_token);

 private:
  WidthId width_;
  std::unique_ptr<Token> data_token_;
};

// directives RESB and RESW
class DirectiveMemReserve : public ExpressionDirective {
 public:
  enum WidthId {
    BYTE = 0,
    WORD
  };

  explicit DirectiveMemReserve(WidthId the_width);
  virtual ~DirectiveMemReserve();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;

  WidthId width() const;
  int32 reservation_size() const;
  void set_reservation_size(int32 size);

 private:
  WidthId width_;
  int32 reservation_size_;
};

class DirectiveInternalLiteral : public Directive {
 public:
  DirectiveInternalLiteral();
  ~DirectiveInternalLiteral();

  static bool ClassOf(const Node* node);
  virtual void AcceptVisitor(NodeVisitor* visitor);
  virtual void AcceptVisitor(ConstNodeVisitor* visitor) const;

  int literal_id() const;
  void set_literal_id(int the_literal_id);

 private:
  int literal_id_;
};

}  // namespace node
}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_NODE_H
