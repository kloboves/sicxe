#include "assembler/parser.h"

#include <assert.h>
#include "assembler/code.h"
#include "assembler/directive.h"
#include "assembler/tokenizer.h"
#include "common/cpu_state.h"
#include "common/error_db.h"
#include "common/instruction.h"
#include "common/instruction_db.h"
#include "common/text_file.h"

using std::string;
using std::unique_ptr;

namespace sicxe {
namespace assembler {

Parser::Config::Config()
    : instruction_db(nullptr), case_sensitive(false), allow_brackets(true) {}

Parser::Parser(const Config* config)
  : config_(config), code_(nullptr), error_db_(nullptr), extended_(false),
    visit_success_(false) {}
Parser::~Parser() {}

bool Parser::ParseFile(const TextFile& file, Code* code, ErrorDB* error_db) {
  code_ = code;
  error_db_ = error_db;
  code_->set_text_file(&file);
  error_db_->SetCurrentFile(&file);

  Tokenizer tokenizer;
  size_t line_count = file.lines().size();
  bool success = true;
  bool seen_start = false;
  bool seen_end = false;
  const Token* last_mnemonic_token = nullptr;
  for (size_t i = 0; i < line_count; i++) {
    tokens_.clear();
    if (!tokenizer.TokenizeLine(file, i, &tokens_, error_db_)) {
      success = false;
      continue;
    }
    if (!ParseLine()) {
      success = false;
      continue;
    }

    if (isa<node::Directive>(*node_)) {
      last_mnemonic_token = cast<node::Directive>(*node_).mnemonic();
    } else if (isa<node::Instruction>(*node_)) {
      last_mnemonic_token = cast<node::Instruction>(*node_).mnemonic();
    }

    if (success) {
      if (!seen_start) {
        if (isa<node::DirectiveStart>(*node_)) {
          seen_start = true;
        } else if (!isa<node::Empty>(*node_)) {
          success = false;
          error_db_->AddError(ErrorDB::ERROR, "code must begin with START directive",
                              last_mnemonic_token->pos());
        }
      } else if (isa<node::DirectiveStart>(*node_)) {
        success = false;
        error_db_->AddError(ErrorDB::ERROR, "only one START directive allowed",
                            last_mnemonic_token->pos());
      }
      if (!seen_end) {
        if (isa<node::DirectiveEnd>(*node_)) {
          seen_end = true;
        }
      } else {
        if (!isa<node::Empty>(*node_)) {
          success = false;
          error_db_->AddError(ErrorDB::ERROR, "no code allowed after END directive",
                              last_mnemonic_token->pos());
        }
      }
    }
    code_->mutable_nodes()->emplace_back(std::move(node_));
    node_ = unique_ptr<node::Node>();
  }
  if (!success) {
    return false;
  }
  if (!seen_start) {
    error_db_->AddError(ErrorDB::ERROR, "file contains no code");
    return false;
  }
  if (!seen_end) {
    error_db_->AddError(ErrorDB::ERROR, "code must end with END directive",
                        last_mnemonic_token->pos());
    return false;
  }
  return true;
}

namespace {

string StringToUppercase(const string& str) {
  string r = str;
  for (size_t i = 0; i < r.size(); i++) {
    if (isalpha(r[i])) {
      r[i] = toupper(r[i]);
    }
  }
  return r;
}

node::Directive* CreateDirectiveNode(Directive::DirectiveId directive_id) {
  switch (directive_id) {
    case Directive::START:
      return new node::DirectiveStart;
    case Directive::END:
      return new node::DirectiveEnd;
    case Directive::ORG:
      return new node::DirectiveOrg;
    case Directive::EQU:
      return new node::DirectiveEqu;
    case Directive::USE:
      return new node::DirectiveUse;
    case Directive::LTORG:
      return new node::DirectiveLtorg;
    case Directive::BASE:
      return new node::DirectiveBase;
    case Directive::NOBASE:
      return new node::DirectiveNobase;
    case Directive::EXTDEF:
      return new node::DirectiveExtdef;
    case Directive::EXTREF:
      return new node::DirectiveExtref;
    case Directive::BYTE:
      return new node::DirectiveMemInit(node::DirectiveMemInit::BYTE);
    case Directive::WORD:
      return new node::DirectiveMemInit(node::DirectiveMemInit::WORD);
    case Directive::RESB:
      return new node::DirectiveMemReserve(node::DirectiveMemReserve::BYTE);
    case Directive::RESW:
      return new node::DirectiveMemReserve(node::DirectiveMemReserve::WORD);
    default:
      assert(false);
  }
  return nullptr;
}

node::Instruction* CreateInstructionNode(Format::FormatId format_id) {
  switch (format_id) {
    case Format::F1:
      return new node::InstructionF1;
    case Format::F2:
      return new node::InstructionF2;
    case Format::FS34:
      return new node::InstructionFS34;
    default:
      assert(false);
  }
  return nullptr;
}

}  // namespace

bool Parser::ParseLine() {
  unique_ptr<Token> label, comment, extended_plus, mnemonic;
  extended_ = false;

  if (!tokens_.empty() && tokens_.back()->type() == Token::COMMENT) {
    comment = std::move(tokens_.back());
    tokens_.pop_back();
  }

  if (tokens_.empty()) {
    node_.reset(new node::Empty);
  } else {
    if (tokens_.front()->type() == Token::NAME && tokens_.front()->pos().column == 0) {
      label = std::move(tokens_.front());
      tokens_.pop_front();
      if (tokens_.empty()) {
        EndOfLineError(*label);
        return false;
      }
    }
    if (tokens_.front()->type() == Token::OPERATOR &&
        tokens_.front()->operator_id() == Token::OP_ADD) {
      extended_plus = std::move(tokens_.front());
      tokens_.pop_front();
      extended_ = true;
      if (tokens_.empty()) {
        EndOfLineError(*extended_plus);
        return false;
      }
    }
    if (tokens_.front()->type() != Token::NAME) {
      error_db_->AddError(ErrorDB::ERROR, "expected mnemonic", tokens_.front()->pos());
      return false;
    }
    mnemonic = std::move(tokens_.front());
    tokens_.pop_front();
    if (extended_ && (mnemonic->pos().column - extended_plus->pos().column) != 1) {
      error_db_->AddError(ErrorDB::ERROR, "whitespace between '+' and mnemonic",
                          extended_plus->pos(), mnemonic->pos());
      return false;
    }

    string mnemonic_str;
    if (config_->case_sensitive) {
      mnemonic_str = mnemonic->value();
    } else {
      mnemonic_str = StringToUppercase(mnemonic->value());
    }

    Directive::DirectiveId directive_id = Directive::START;
    const Instruction* instruction = nullptr;
    if (Directive::NameToId(mnemonic_str, &directive_id)) {
      if (extended_) {
        error_db_->AddError(ErrorDB::ERROR, "cannot use '+' with directive",
                            extended_plus->pos());
        return false;
      }

      node::Directive* directive_node = CreateDirectiveNode(directive_id);
      directive_node->set_mnemonic(mnemonic.release());
      node_.reset(directive_node);
    } else if ((instruction = config_->instruction_db->FindMnemonic(mnemonic_str))
               != nullptr) {
      if (extended_ && instruction->format() != Format::FS34) {
        error_db_->AddError(ErrorDB::ERROR, "cannot use '+' with this instruction",
                            extended_plus->pos());
        return false;
      }

      node::Instruction* node_instruction = CreateInstructionNode(instruction->format());
      node_instruction->set_opcode(instruction->opcode());
      node_instruction->set_syntax(instruction->syntax());
      node_instruction->set_mnemonic(mnemonic.release());
      node_.reset(node_instruction);
    } else {
      error_db_->AddError(ErrorDB::ERROR, "invalid mnemonic", mnemonic->pos());
      return false;
    }
    node_->set_label(label.release());
  }
  node_->set_comment(comment.release());

  visit_success_ = true;
  node_->AcceptVisitor(this);
  if (!visit_success_) {
    return false;
  }

  if (!tokens_.empty()) {
    error_db_->AddError(ErrorDB::ERROR, "unexpected tokens at end of line",
                        tokens_.front()->pos(), tokens_.back()->pos());
    return false;
  }
  return true;
}

void Parser::VisitNode(node::Empty*) {}

void Parser::VisitNode(node::InstructionF1*) {}

bool Parser::ParseRegisterName(const Token& token, uint8* register_id) {
  if (token.type() != Token::NAME) {
    error_db_->AddError(ErrorDB::ERROR, "expected register name", token.pos());
    return false;
  }
  CpuState::RegisterId id;
  if (!CpuState::RegisterNameToId(token.value(), &id)) {
    error_db_->AddError(ErrorDB::ERROR, "invalid register name", token.pos());
    return false;
  }
  *register_id = static_cast<uint8>(id);
  return true;
}

void Parser::VisitNode(node::InstructionF2* node) {
  uint8 r1 = 0, r2 = 0;
  unique_ptr<Token> operand1, operand2;
  if (tokens_.empty()) {
    EndOfLineError(*node->mnemonic());
    visit_success_ = false;
    return;
  }
  operand1 = std::move(tokens_.front());
  tokens_.pop_front();
  if (!ParseRegisterName(*operand1, &r1)) {
    visit_success_ = false;
    return;
  }
  node->set_r1(r1);

  if (node->syntax() == Syntax::F2_REG_REG || node->syntax() == Syntax::F2_REG_N) {
    unique_ptr<Token> comma;
    if (tokens_.empty()) {
      EndOfLineError(*operand1);
      visit_success_ = false;
      return;
    }
    comma = std::move(tokens_.front());
    tokens_.pop_front();
    if (comma->type() != Token::OPERATOR || comma->operator_id() != Token::OP_COMMA) {
      error_db_->AddError(ErrorDB::ERROR, "expected ','", comma->pos());
      visit_success_ = false;
      return;
    }
    if (tokens_.empty()) {
      EndOfLineError(*comma);
      visit_success_ = false;
      return;
    }
    operand2 = std::move(tokens_.front());
    tokens_.pop_front();
    if (node->syntax() == Syntax::F2_REG_REG) {
      if (!ParseRegisterName(*operand2, &r2)) {
        visit_success_ = false;
        return;
      }
    } else {
      if (operand2->type() != Token::INTEGER) {
        error_db_->AddError(ErrorDB::ERROR, "expected integer", operand2->pos());
        visit_success_ = false;
        return;
      }
      if (operand2->integer() >= 16) {
        error_db_->AddError(ErrorDB::ERROR, "operand must be less than 16",
                            operand2->pos());
        visit_success_ = false;
        return;
      }
      r2 = static_cast<uint8>(operand2->integer() & 0xf);
    }
    node->set_r2(r2);
  }
}

bool Parser::ParseExpression(bool allow_brackets, TokenList* expression) {
  assert(!tokens_.empty());
  unique_ptr<Token> bracket_start;
  if (allow_brackets && tokens_.front()->type() == Token::OPERATOR &&
      tokens_.front()->operator_id() == Token::OP_LBRACKET) {
    bracket_start = std::move(tokens_.front());
    tokens_.pop_front();
    if (tokens_.empty()) {
      EndOfLineError(*bracket_start);
      return false;
    }
  }

  // allow one '-' before expression
  if (tokens_.front()->type() == Token::OPERATOR &&
      tokens_.front()->operator_id() == Token::OP_SUB) {
    expression->emplace_back(std::move(tokens_.front()));
    tokens_.pop_front();
    if (tokens_.empty()) {
      EndOfLineError(*expression->back());
      return false;
    }
  }

  while (true) {
    const Token& value_token = *tokens_.front();
    if (value_token.type() != Token::NAME && value_token.type() != Token::INTEGER) {
      error_db_->AddError(ErrorDB::ERROR, "expected symbol name or integer",
                          value_token.pos());
      return false;
    }
    expression->emplace_back(std::move(tokens_.front()));
    tokens_.pop_front();
    if (tokens_.empty()) {
      break;
    }

    const Token& operator_token = *tokens_.front();
    if (operator_token.type() == Token::OPERATOR) {
      if (operator_token.operator_id() >= Token::OP_ADD &&
          operator_token.operator_id() <= Token::OP_DIV) {
        expression->emplace_back(std::move(tokens_.front()));
        tokens_.pop_front();
      } else {
        break;
      }
    } else {
      error_db_->AddError(ErrorDB::ERROR, "expected operator", operator_token.pos());
      return false;
    }
    if (tokens_.empty()) {
      EndOfLineError(operator_token);
      return false;
    }
  }

  if (bracket_start != nullptr) {
    if (tokens_.empty()) {
      error_db_->AddError(ErrorDB::ERROR, "unmatched '['", bracket_start->pos());
      return false;
    } else {
      unique_ptr<Token> bracket_end = std::move(tokens_.front());
      tokens_.pop_front();
      if (bracket_end->type() != Token::OPERATOR ||
          bracket_end->operator_id() != Token::OP_RBRACKET) {
        error_db_->AddError(ErrorDB::ERROR, "expected ']' or arithmetic operator",
                            bracket_end->pos());
        return false;
      }
    }
  }
  return true;
}

namespace {

node::InstructionFS34::AddressingId AddressingFromOperatorToken(const Token& token) {
  switch (token.operator_id()) {
    case Token::OP_ASSIGN:
      return node::InstructionFS34::LITERAL_POOL;
    case Token::OP_HASH:
      return node::InstructionFS34::IMMEDIATE;
    case Token::OP_AT:
      return node::InstructionFS34::INDIRECT;
    default:
      assert(false);
  }
  return node::InstructionFS34::SIMPLE;
}

string* CreateIndexRegName() {
  string* name = new string;
  bool success = CpuState::RegisterIdToName(CpuState::REG_X, name);
  assert(success);
  unused(success);
  return name;
}

}  // namespace

bool Parser::ParseAddressingOperator(node::InstructionFS34* node) {
  unique_ptr<Token> addressing_operator;
  node::InstructionFS34::AddressingId addressing = node::InstructionFS34::SIMPLE;
  if (tokens_.front()->type() == Token::OPERATOR &&
      tokens_.front()->operator_id() >= Token::OP_ASSIGN &&
      tokens_.front()->operator_id() <= Token::OP_AT) {
    addressing_operator = std::move(tokens_.front());
    tokens_.pop_front();
    addressing = AddressingFromOperatorToken(*addressing_operator);
    if (tokens_.empty()) {
      EndOfLineError(*addressing_operator);
      return false;
    }
  }
  node->set_addressing(addressing);

  if (node->syntax() == Syntax::FS34_STORE_B || node->syntax() == Syntax::FS34_STORE_W ||
      node->syntax() == Syntax::FS34_STORE_F) {
    if (addressing == node::InstructionFS34::IMMEDIATE) {
      error_db_->AddError(ErrorDB::ERROR,
                          "immediate addressing not allowed with store instructions",
                          addressing_operator->pos());
      return false;
    }
    if (addressing == node::InstructionFS34::LITERAL_POOL) {
      error_db_->AddError(ErrorDB::ERROR,
                          "literal pool addressing not allowed with store instructions",
                          addressing_operator->pos());
      return false;
    }
  }

  if (node->syntax() == Syntax::FS34_LOAD_F &&
      addressing == node::InstructionFS34::IMMEDIATE) {
    error_db_->AddError(ErrorDB::ERROR,
                        "immediate addressing not allowed with floating "
                        "point instructions", addressing_operator->pos());
    return false;
  }
  return true;
}

bool Parser::CheckDataTokenMaxSize(const Token& data_token, Syntax::SyntaxId syntax) {
  if (syntax == Syntax::FS34_LOAD_B) {
    if (data_token.data().size() > 1) {
      error_db_->AddError(ErrorDB::ERROR, "literal size is limited to 1 byte",
                          data_token.pos());
      return false;
    }
  } else if (syntax == Syntax::FS34_LOAD_W) {
    if (data_token.data().size() > 3) {
      error_db_->AddError(ErrorDB::ERROR, "literal size is limited to 3 bytes",
                          data_token.pos());
      return false;
    }
  } else if (syntax == Syntax::FS34_LOAD_F) {
    if (data_token.data().size() > 6) {
      error_db_->AddError(ErrorDB::ERROR, "literal size is limited to 6 bytes",
                          data_token.pos());
      return false;
    }
  } else {
    assert(false);
  }
  return true;
}

bool Parser::ParseIndexOperator(node::InstructionFS34* node) {
  unique_ptr<Token> comma = std::move(tokens_.front());
  tokens_.pop_front();
  if (comma->type() != Token::OPERATOR || comma->operator_id() != Token::OP_COMMA) {
    error_db_->AddError(ErrorDB::ERROR, "expected ','", comma->pos());
    return false;
  }
  if (tokens_.empty()) {
    EndOfLineError(*comma);
    return false;
  }
  static const string* index_reg_name = CreateIndexRegName();
  unique_ptr<Token> index_reg_token = std::move(tokens_.front());
  tokens_.pop_front();
  if (index_reg_token->type() != Token::NAME ||
      index_reg_token->value() != *index_reg_name) {
    error_db_->AddError(ErrorDB::ERROR, ("expected '" + *index_reg_name + "'").c_str(),
                        index_reg_token->pos());
    return false;
  }
  if (node->addressing() != node::InstructionFS34::SIMPLE) {
    error_db_->AddError(ErrorDB::ERROR,
                        "indexed addressing allowed only when using simple addressing",
                        index_reg_token->pos());
    return false;
  }
  node->set_indexed(true);
  return true;
}

void Parser::VisitNode(node::InstructionFS34* node) {
  if (node->syntax() == Syntax::FS34_NONE) {
    return;
  }
  if (tokens_.empty()) {
    EndOfLineError(*node->mnemonic());
    visit_success_ = false;
    return;
  }
  node->set_extended(extended_);
  if (!ParseAddressingOperator(node)) {
    visit_success_ = false;
    return;
  }

  node::InstructionFS34::AddressingId addressing = node->addressing();
  if (addressing == node::InstructionFS34::LITERAL_POOL) {
    if (node->syntax() == Syntax::FS34_LOAD_F) {
      unique_ptr<Token> data_token = std::move(tokens_.front());
      tokens_.pop_front();
      if (data_token->type() != Token::DATA_FLOAT &&
          data_token->type() != Token::DATA_BIN) {
        error_db_->AddError(ErrorDB::ERROR, "expected data constant", data_token->pos());
        visit_success_ = false;
        return;
      }
      if (!CheckDataTokenMaxSize(*data_token, node->syntax())) {
        visit_success_ = false;
        return;
      }
      node->set_data_token(data_token.release());
    } else if (node->syntax() == Syntax::FS34_LOAD_B ||
               node->syntax() == Syntax::FS34_LOAD_W) {
      if (tokens_.front()->type() == Token::DATA_FLOAT) {
        error_db_->AddError(ErrorDB::ERROR,
                            "float literal not allowed with this instruction",
                            tokens_.front()->pos());
        visit_success_ = false;
        return;
      }
      if (tokens_.front()->type() == Token::DATA_BIN) {
        unique_ptr<Token> data_token = std::move(tokens_.front());
        tokens_.pop_front();
        if (!CheckDataTokenMaxSize(*data_token, node->syntax())) {
          visit_success_ = false;
          return;
        }
        node->set_data_token(data_token.release());
      } else {
        if (!ParseExpression(config_->allow_brackets, node->mutable_expression())) {
          visit_success_ = false;
          return;
        }
      }
    } else {
      assert(false);
    }
  } else {
    bool brackets = config_->allow_brackets &&
                    (addressing == node::InstructionFS34::IMMEDIATE ||
                     addressing == node::InstructionFS34::INDIRECT);
    if (!ParseExpression(brackets, node->mutable_expression())) {
      visit_success_ = false;
      return;
    }
  }

  if (tokens_.empty()) {
    return;
  }
  if (!ParseIndexOperator(node)) {
    visit_success_ = false;
    return;
  }
}

void Parser::VisitExpressionDirective(node::ExpressionDirective* node) {
  if (tokens_.empty()) {
    EndOfLineError(*node->mnemonic());
    visit_success_ = false;
    return;
  }
  if (!ParseExpression(false, node->mutable_expression())) {
    visit_success_ = false;
    return;
  }
}

void Parser::VisitNode(node::DirectiveStart* node) {
  if (node->label() == nullptr) {
    error_db_->AddError(ErrorDB::ERROR, "program name required before START directive",
                        node->mnemonic()->pos());
    visit_success_ = false;
    return;
  }
  if (node->label()->value().size() > 6) {
    error_db_->AddError(ErrorDB::ERROR, "program name length is limited to 6 characters",
                        node->label()->pos());
    visit_success_ = false;
    return;
  }
  VisitExpressionDirective(node);
}

void Parser::VisitNode(node::DirectiveEnd* node) {
  if (node->label() != nullptr) {
    LabelNotAllowedError(*node->label());
    visit_success_ = false;
    return;
  }
  VisitExpressionDirective(node);
}

void Parser::VisitNode(node::DirectiveOrg* node) {
  if (node->label() != nullptr) {
    LabelNotAllowedError(*node->label());
    visit_success_ = false;
    return;
  }
  VisitExpressionDirective(node);
}

void Parser::VisitNode(node::DirectiveEqu* node) {
  if (node->label() == nullptr) {
    error_db_->AddError(ErrorDB::ERROR, "symbol name required before EQU directive",
                        node->mnemonic()->pos());
    visit_success_ = false;
    return;
  }
  if (!tokens_.empty() && tokens_.front()->type() == Token::OPERATOR &&
      tokens_.front()->operator_id() == Token::OP_MUL) {
    tokens_.pop_front();
    node->set_assign_current_address(true);
  } else {
    VisitExpressionDirective(node);
  }
}

void Parser::VisitNode(node::DirectiveUse* node) {
  if (node->label() != nullptr) {
    LabelNotAllowedError(*node->label());
    visit_success_ = false;
    return;
  }
  if (!tokens_.empty()) {
    unique_ptr<Token> block_name(std::move(tokens_.front()));
    tokens_.pop_front();
    if (block_name->type() != Token::NAME) {
      error_db_->AddError(ErrorDB::ERROR, "expected block name", block_name->pos());
      visit_success_ = false;
      return;
    }
    node->set_block_name(block_name.release());
  }
}

void Parser::VisitNode(node::DirectiveLtorg* node) {
  if (node->label() != nullptr) {
    LabelNotAllowedError(*node->label());
    visit_success_ = false;
    return;
  }
}

void Parser::VisitNode(node::DirectiveBase* node) {
  if (node->label() != nullptr) {
    LabelNotAllowedError(*node->label());
    visit_success_ = false;
    return;
  }
  VisitExpressionDirective(node);
}

void Parser::VisitNode(node::DirectiveNobase* node) {
  if (node->label() != nullptr) {
    LabelNotAllowedError(*node->label());
    visit_success_ = false;
    return;
  }
}

void Parser::VisitSymbolListDirective(node::SymbolListDirective* node) {
  if (tokens_.empty()) {
    EndOfLineError(*node->mnemonic());
    visit_success_ = false;
    return;
  }

  unique_ptr<Token> token;
  while (true) {
    token = std::move(tokens_.front());
    tokens_.pop_front();
    if (token->type() != Token::NAME) {
      error_db_->AddError(ErrorDB::ERROR, "expected symbol name", token->pos());
      visit_success_ = false;
      return;
    }
    if (token->value().size() > 6) {
      error_db_->AddError(ErrorDB::ERROR,
                          "external symbol name length is limited to 6 characters",
                          token->pos());
      visit_success_ = false;
    }
    node->mutable_symbol_list()->emplace_back(std::move(token));
    if (tokens_.empty()) {
      break;
    }

    token = std::move(tokens_.front());
    tokens_.pop_front();

    if (token->type() != Token::OPERATOR || token->operator_id() != Token::OP_COMMA) {
      error_db_->AddError(ErrorDB::ERROR, "expected ','", token->pos());
      visit_success_ = false;
      return;
    }
    if (tokens_.empty()) {
      EndOfLineError(*token);
      visit_success_ = false;
      return;
    }
  }
}

void Parser::VisitNode(node::DirectiveExtdef* node) {
  if (node->label() != nullptr) {
    LabelNotAllowedError(*node->label());
    visit_success_ = false;
    return;
  }
  VisitSymbolListDirective(node);
}

void Parser::VisitNode(node::DirectiveExtref* node) {
  if (node->label() != nullptr) {
    LabelNotAllowedError(*node->label());
    visit_success_ = false;
    return;
  }
  VisitSymbolListDirective(node);
}

void Parser::VisitNode(node::DirectiveMemInit* node) {
  if (!tokens_.empty() &&
      (tokens_.front()->type() == Token::DATA_BIN ||
       tokens_.front()->type() == Token::DATA_FLOAT)) {
    node->set_data_token(tokens_.front().release());
    tokens_.pop_front();
  } else {
    VisitExpressionDirective(node);
  }
}

void Parser::VisitNode(node::DirectiveMemReserve* node) {
  VisitExpressionDirective(node);
}

void Parser::VisitNode(node::DirectiveInternalLiteral*) {
  assert(false);
}

void Parser::EndOfLineError(const Token& last_token) {
  const TextFile::Position& pos = last_token.pos();
  error_db_->AddError(ErrorDB::ERROR, "unexpected end of line",
                      TextFile::Position(pos.row, pos.column + pos.size - 1, 1));
}

void Parser::LabelNotAllowedError(const Token& label) {
  error_db_->AddError(ErrorDB::ERROR, "label not allowed before this directive",
                      label.pos());
}

}  // namespace assembler
}  // namespace sicxe
