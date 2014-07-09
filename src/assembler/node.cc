#include "assembler/node.h"

#include "assembler/node_visitor.h"

namespace sicxe {
namespace assembler {
namespace node {

// Node implementation
Node::Node(NodeKind the_kind) : kind_(the_kind) {}
Node::~Node() {}

Node::NodeKind Node::kind() const {
  return kind_;
}

const Token* Node::label() const {
  return label_.get();
}

void Node::set_label(Token* the_label) {
  label_.reset(the_label);
}

const Token* Node::comment() const {
  return comment_.get();
}

void Node::set_comment(Token* the_comment) {
  comment_.reset(the_comment);
}

// Empty implementation
Empty::Empty() : Node(NK_Empty) {};
Empty::~Empty() {};

bool Empty::ClassOf(const Node* node) {
  return node->kind() == NK_Empty;
}

void Empty::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void Empty::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}


// Instruction implementation
Instruction::Instruction(NodeKind the_kind, Format::FormatId the_format)
    : Node(the_kind), format_(the_format), opcode_(0), syntax_(Syntax::F1_NONE) {}
Instruction::~Instruction() {}

bool Instruction::ClassOf(const Node* node) {
  return node->kind() >= NK_Instruction && node->kind() <= NK_Instruction_Last;
}

Format::FormatId Instruction::format() const {
  return format_;
}

uint8 Instruction::opcode() const {
  return opcode_;
}

void Instruction::set_opcode(uint8 the_opcode) {
  opcode_ = the_opcode;
}

Syntax::SyntaxId Instruction::syntax() const {
  return syntax_;
}

void Instruction::set_syntax(Syntax::SyntaxId id) {
  syntax_ = id;
}

const Token* Instruction::mnemonic() const {
  return mnemonic_.get();
}

void Instruction::set_mnemonic(Token* the_mnemonic) {
  mnemonic_.reset(the_mnemonic);
}

// InstructionF1 implementation
InstructionF1::InstructionF1() : Instruction(NK_InstructionF1, Format::F1) {}
InstructionF1::~InstructionF1() {}

bool InstructionF1::ClassOf(const Node* node) {
  return node->kind() == NK_InstructionF1;
}

void InstructionF1::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void InstructionF1::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

// InstructionF2 implementation
InstructionF2::InstructionF2()
    : Instruction(NK_InstructionF2, Format::F2), r1_(0), r2_(0) {}
InstructionF2::~InstructionF2() {}

bool InstructionF2::ClassOf(const Node* node) {
  return node->kind() == NK_InstructionF2;
}

void InstructionF2::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void InstructionF2::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

uint8 InstructionF2::r1() const {
  return r1_;
}

void InstructionF2::set_r1(uint8 value) {
  r1_ = value;
}

uint8 InstructionF2::r2() const {
  return r2_;
}

void InstructionF2::set_r2(uint8 value) {
  r2_ = value;
}

// InstructionFS34 implementation
InstructionFS34::InstructionFS34()
    : Instruction(NK_InstructionFS34, Format::FS34), addressing_(SIMPLE),
      extended_(false), indexed_(false), literal_id_(-1) {}
InstructionFS34::~InstructionFS34() {}

bool InstructionFS34::ClassOf(const Node* node) {
  return node->kind() == NK_InstructionFS34;
}

void InstructionFS34::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void InstructionFS34::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

const Node::TokenList& InstructionFS34::expression() const {
  return expression_;
}

Node::TokenList* InstructionFS34::mutable_expression() {
  return &expression_;
}

const Token* InstructionFS34::data_token() const {
  return data_token_.get();
}

void InstructionFS34::set_data_token(Token* the_data_token) {
  data_token_.reset(the_data_token);
}

InstructionFS34::AddressingId InstructionFS34::addressing() const {
  return addressing_;
}

void InstructionFS34::set_addressing(AddressingId the_addressing) {
  addressing_ = the_addressing;
}

bool InstructionFS34::extended() const {
  return extended_;
}

void InstructionFS34::set_extended(bool value) {
  extended_ = value;
}

bool InstructionFS34::indexed() const {
  return indexed_;
}

void InstructionFS34::set_indexed(bool value) {
  indexed_ = value;
}

int InstructionFS34::literal_id() const {
  return literal_id_;
}

void InstructionFS34::set_literal_id(int the_literal_id) {
  literal_id_ = the_literal_id;
}

// Directive implementation
Directive::Directive(NodeKind the_kind, DirectiveId the_id)
    : Node(the_kind), directive_id_(the_id) {}
Directive::~Directive() {}

bool Directive::ClassOf(const Node* node) {
  return node->kind() >= NK_Directive && node->kind() <= NK_Directive_Last;
}

Directive::DirectiveId Directive::directive_id() const {
  return directive_id_;
}

const Token* Directive::mnemonic() const {
  return mnemonic_.get();
}

void Directive::set_mnemonic(Token* the_mnemonic) {
  mnemonic_.reset(the_mnemonic);
}

// ExpressionDirective implementation
ExpressionDirective::ExpressionDirective(NodeKind the_kind, DirectiveId the_id)
    : Directive(the_kind, the_id) {}
ExpressionDirective::~ExpressionDirective() {}

bool ExpressionDirective::ClassOf(const Node* node) {
  return node->kind() >= NK_ExpressionDirective &&
         node->kind() <= NK_ExpressionDirective_Last;
}

const Node::TokenList& ExpressionDirective::expression() const {
  return expression_;
}

Node::TokenList* ExpressionDirective::mutable_expression() {
  return &expression_;
}

// SymbolListDirective implementation
SymbolListDirective::SymbolListDirective(NodeKind the_kind, DirectiveId the_id)
    : Directive(the_kind, the_id) {}
SymbolListDirective::~SymbolListDirective() {}

bool SymbolListDirective::ClassOf(const Node* node) {
  return node->kind() >= NK_SymbolListDirective &&
         node->kind() <= NK_SymbolListDirective_Last;
}

const Node::TokenList& SymbolListDirective::symbol_list() const {
  return symbol_list_;
}

Node::TokenList* SymbolListDirective::mutable_symbol_list() {
  return &symbol_list_;
}

// DirectiveStart implementation
DirectiveStart::DirectiveStart()
    : ExpressionDirective(NK_DirectiveStart, assembler::Directive::START) {}
DirectiveStart::~DirectiveStart() {}

bool DirectiveStart::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveStart;
}

void DirectiveStart::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveStart::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

// DirectiveEnd implementation
DirectiveEnd::DirectiveEnd()
    : ExpressionDirective(NK_DirectiveEnd, assembler::Directive::END) {}
DirectiveEnd::~DirectiveEnd() {}

bool DirectiveEnd::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveEnd;
}

void DirectiveEnd::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveEnd::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

// DirectiveOrg implementation
DirectiveOrg::DirectiveOrg()
    : ExpressionDirective(NK_DirectiveOrg, assembler::Directive::ORG), address_(0) {}
DirectiveOrg::~DirectiveOrg() {}

bool DirectiveOrg::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveOrg;
}

void DirectiveOrg::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveOrg::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

uint32 DirectiveOrg::address() const {
  return address_;
}

void DirectiveOrg::set_address(uint32 the_address) {
  address_ = the_address;
}

// DirectiveEqu implementation
DirectiveEqu::DirectiveEqu()
    : ExpressionDirective(NK_DirectiveEqu, assembler::Directive::EQU),
      assign_current_address_(false) {}
DirectiveEqu::~DirectiveEqu() {}

bool DirectiveEqu::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveEqu;
}

void DirectiveEqu::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveEqu::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

bool DirectiveEqu::assign_current_address() const {
  return assign_current_address_;
}

void DirectiveEqu::set_assign_current_address(bool value) {
  assign_current_address_ = value;
}

// DirectiveUse implementation
DirectiveUse::DirectiveUse()
    : Directive(NK_DirectiveUse, assembler::Directive::USE), block_id_(-1) {}
DirectiveUse::~DirectiveUse() {}

bool DirectiveUse::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveUse;
}

void DirectiveUse::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveUse::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

int DirectiveUse::block_id() const {
  return block_id_;
}

void DirectiveUse::set_block_id(int id) {
  block_id_ = id;
}

const Token* DirectiveUse::block_name() const {
  return block_name_.get();
}

void DirectiveUse::set_block_name(Token* the_block_name) {
  block_name_.reset(the_block_name);
}

// DirectiveLtorg implementation
DirectiveLtorg::DirectiveLtorg()
    : Directive(NK_DirectiveLtorg, assembler::Directive::LTORG) {}
DirectiveLtorg::~DirectiveLtorg() {}

bool DirectiveLtorg::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveLtorg;
}

void DirectiveLtorg::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveLtorg::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

// DirectiveBase implementation
DirectiveBase::DirectiveBase()
    : ExpressionDirective(NK_DirectiveBase, assembler::Directive::BASE) {}
DirectiveBase::~DirectiveBase() {}

bool DirectiveBase::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveBase;
}

void DirectiveBase::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveBase::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

// DirectiveNobase implementation
DirectiveNobase::DirectiveNobase()
    : Directive(NK_DirectiveBase, assembler::Directive::NOBASE) {}
DirectiveNobase::~DirectiveNobase() {}

bool DirectiveNobase::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveNobase;
}

void DirectiveNobase::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveNobase::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

// DirectiveExtdef implementation
DirectiveExtdef::DirectiveExtdef()
    : SymbolListDirective(NK_DirectiveExtdef, assembler::Directive::EXTDEF) {}
DirectiveExtdef::~DirectiveExtdef() {}

bool DirectiveExtdef::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveExtdef;
}

void DirectiveExtdef::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveExtdef::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

// DirectiveExtref implementation
DirectiveExtref::DirectiveExtref()
    : SymbolListDirective(NK_DirectiveExtref, assembler::Directive::EXTREF) {}
DirectiveExtref::~DirectiveExtref() {}

bool DirectiveExtref::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveExtref;
}

void DirectiveExtref::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveExtref::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

// DirectiveMemInit implementation
DirectiveMemInit::DirectiveMemInit(WidthId the_width)
    : ExpressionDirective(NK_DirectiveMemInit,
        (the_width == WORD) ? assembler::Directive::WORD : assembler::Directive::BYTE),
      width_(the_width) {}
DirectiveMemInit::~DirectiveMemInit() {}

bool DirectiveMemInit::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveMemInit;
}

void DirectiveMemInit::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveMemInit::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

DirectiveMemInit::WidthId DirectiveMemInit::width() const {
  return width_;
}

const Token* DirectiveMemInit::data_token() const {
  return data_token_.get();
}

void DirectiveMemInit::set_data_token(Token* the_data_token) {
  data_token_.reset(the_data_token);
}

// DirectiveMemReserve implementation
DirectiveMemReserve::DirectiveMemReserve(WidthId the_width)
    : ExpressionDirective(NK_DirectiveMemReserve,
        (the_width == WORD) ? assembler::Directive::RESW : assembler::Directive::RESB),
      width_(the_width), reservation_size_(0) {}
DirectiveMemReserve::~DirectiveMemReserve() {}

bool DirectiveMemReserve::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveMemReserve;
}

void DirectiveMemReserve::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveMemReserve::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

DirectiveMemReserve::WidthId DirectiveMemReserve::width() const {
  return width_;
}

int32 DirectiveMemReserve::reservation_size() const {
  return reservation_size_;
}

void DirectiveMemReserve::set_reservation_size(int32 size) {
  reservation_size_ = size;
}

// DirectiveInternalLiteral implementation
DirectiveInternalLiteral::DirectiveInternalLiteral()
    : Directive(NK_DirectiveInternalLiteral, assembler::Directive::INTERNAL_LITERAL),
      literal_id_(-1) {}
DirectiveInternalLiteral::~DirectiveInternalLiteral() {}

bool DirectiveInternalLiteral::ClassOf(const Node* node) {
  return node->kind() == NK_DirectiveInternalLiteral;
}

void DirectiveInternalLiteral::AcceptVisitor(NodeVisitor* visitor) {
  visitor->VisitNode(this);
}

void DirectiveInternalLiteral::AcceptVisitor(ConstNodeVisitor* visitor) const {
  visitor->VisitNode(this);
}

int DirectiveInternalLiteral::literal_id() const {
  return literal_id_;
}

void DirectiveInternalLiteral::set_literal_id(int the_literal_id) {
  literal_id_ = the_literal_id;
}

}  // namespace node
}  // namespace assembler
}  // namespace sicxe
