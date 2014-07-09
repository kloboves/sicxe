#include "assembler/table_builder.h"

#include <algorithm>
#include <memory>
#include "assembler/block_table.h"
#include "assembler/expression_util.h"
#include "assembler/literal_table.h"
#include "assembler/node.h"
#include "assembler/symbol_table.h"
#include "assembler/token.h"
#include "common/error_db.h"
#include "common/format.h"

using std::make_pair;
using std::pair;
using std::sort;
using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {
namespace assembler {

TableBuilder::TableBuilder()
    : code_(nullptr), error_db_(nullptr), start_address_(0), entry_point_(0),
      segment_start_(0), current_block_(-1), current_address_(0), success_(false),
      next_literal_id_(0) {}
TableBuilder::~TableBuilder() {}

bool TableBuilder::BuildTables(Code* code, ErrorDB* error_db) {
  code_ = code;
  symbol_table_ .reset(new SymbolTable);
  block_table_.reset(new BlockTable);
  literal_table_.reset(new LiteralTable);
  error_db_ = error_db;
  error_db_->SetCurrentFile(code_->text_file());

  start_address_ = 0;
  entry_point_ = 0;
  segments_.clear();
  undefined_symbol_tokens_.clear();
  segment_start_ = 0;
  current_block_ = -1;
  current_address_ = 0;
  success_ = true;
  next_literal_id_ = 0;

  Code::NodeList* nodes = code_->mutable_nodes();
  for (it_ = nodes->begin(); it_ != nodes->end(); ++it_) {
    (*it_)->AcceptVisitor(this);
    if (!success_ || !CheckAddressOverflow(current_address_)) {
      return false;
    }
  }

  if (!OrderBlocks() || !CheckIntersect()) {
    return false;
  }
  bool has_undefined_symbols = false;
  FinalizeSymbolTable(&has_undefined_symbols);
  if (has_undefined_symbols) {
    UndefinedSymbolsError();
    return false;
  }
  uint32 end_address = GetEndAddress();
  bool entry_point_valid = false;
  if (end_address == start_address_) {
    entry_point_valid = (entry_point_ == start_address_);
  } else {
    entry_point_valid = entry_point_ >= start_address_ &&
                        entry_point_ < end_address;
  }
  if (!entry_point_valid) {
    error_db_->AddError(ErrorDB::ERROR,
                        "entry point must be between start and end address");
    return false;
  }
  code->set_start_address(start_address_);
  code->set_end_address(end_address);
  code->set_entry_point(entry_point_);
  code_->set_symbol_table(symbol_table_.release());
  code_->set_block_table(block_table_.release());
  code_->set_literal_table(literal_table_.release());
  return true;
}

void TableBuilder::VisitNode(node::Empty*) {}

void TableBuilder::VisitNode(node::InstructionF1* node) {
  if (node->label() != nullptr) {
    if (!DefineSymbol(*node->label())) {
      success_ = false;
      return;
    }
  }
  current_address_ += 1;
}

void TableBuilder::VisitNode(node::InstructionF2* node) {
  if (node->label() != nullptr) {
    if (!DefineSymbol(*node->label())) {
      success_ = false;
      return;
    }
  }
  current_address_ += 2;
}

void TableBuilder::VisitNode(node::InstructionFS34* node) {
  if (node->label() != nullptr) {
    if (!DefineSymbol(*node->label())) {
      success_ = false;
      return;
    }
  }
  if (node->extended()) {
    current_address_ += 4;
  } else {
    current_address_ += 3;
  }

  if (node->addressing() == node::InstructionFS34::LITERAL_POOL) {
    int literal_id = 0;
    if (node->data_token() != nullptr) {
      switch (node->syntax()) {
        case Syntax::FS34_LOAD_B:
          literal_id = literal_table_->NewLiteralByte(node->data_token()->data());
          break;
        case Syntax::FS34_LOAD_W:
          literal_id = literal_table_->NewLiteralWord(node->data_token()->data());
          break;
        case Syntax::FS34_LOAD_F:
          literal_id = literal_table_->NewLiteralFloat(node->data_token()->data());
          break;
        default:
          assert(false);
          break;
      }
    } else {
      int32 value = 0;
      const bool is_byte = (node->syntax() == Syntax::FS34_LOAD_B);
      int32 limit_min = is_byte ? -(1 << 7) : -(1 << 23);
      int32 limit_max = is_byte ? (1 << 8) - 1 : (1 << 24) - 1;
      if (!SolveAbsoluteExpression(node->expression(), limit_min, limit_max, &value)) {
        success_ = false;
        return;
      }
      value &= limit_max;
      switch (node->syntax()) {
        case Syntax::FS34_LOAD_B:
          literal_id = literal_table_->NewLiteralByte(static_cast<uint8>(value));
          break;
        case Syntax::FS34_LOAD_W:
          literal_id = literal_table_->NewLiteralWord(value);
          break;
        default:
          assert(false);
          break;
      }
    }
    node->set_literal_id(literal_id);
    string symbol_name = LiteralTable::LiteralSymbolName(literal_id);
    ReferenceSymbol(symbol_name);
  } else {
    ReferenceExpression(node->expression());
  }
}

void TableBuilder::VisitNode(node::DirectiveStart* node) {
  assert(node->label() != nullptr);
  code_->set_program_name(node->label()->value());
  int32 value = 0;
  if (!SolveAbsoluteExpression(node->expression(), 0, 0xfffff, &value)) {
    success_ = false;
    return;
  }
  // set start address
  start_address_ = value;
  BlockTable::Entry* block_entry = block_table_->GetBlock(0);
  block_entry->start = value;
  current_address_ = 0;
  current_block_ = 0;
  // create symbol
  SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(node->label()->value());
  assert(entry->type == SymbolTable::UNKNOWN);
  entry->type = SymbolTable::INTERNAL;
  entry->relative = true;
  entry->defined = true;
  entry->resolved = true;
  entry->exported = true;
  entry->address = value;
  entry->block = 0;
  entry->block_address = 0;
}

void TableBuilder::VisitNode(node::DirectiveEnd* node) {
  InsertLiterals(false);
  EndSegment();
  int32 value = 0;
  if (!SolveRelativeExpression(node->expression(), 0, 0xfffff, &value)) {
    success_ = false;
    return;
  }
  entry_point_ = value;
}

void TableBuilder::VisitNode(node::DirectiveOrg* node) {
  const node::Node::TokenList& expression = node->expression();
  EndSegment();
  int32 value = 0;
  if (!SolveAbsoluteExpression(expression, 0, 0xfffff, &value)) {
    success_ = false;
    return;
  }
  if (value < static_cast<int32>(start_address_)) {
    error_db_->AddError(ErrorDB::ERROR,
                        "expression value must not be less than code start address",
                        expression.front()->pos(), expression.back()->pos());
    success_ = false;
    return;
  }
  current_block_ = -1;
  current_address_ = value;
  segment_start_ = value;
  node->set_address(value);
}

void TableBuilder::VisitNode(node::DirectiveEqu* node) {
  assert(node->label() != nullptr);
  if (node->assign_current_address()) {
    if (!DefineSymbol(*node->label())) {
      success_ = false;
    }
    return;
  }
  int32 value = 0;
  bool relative = false;
  if (!ExpressionUtil::Solve(node->expression(), *symbol_table_, false,
                             &value, &relative, nullptr, error_db_) ||
      !CheckExpressionLimits(0, 0xfffff, value, node->expression())) {
    success_ = false;
    return;
  }
  SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(node->label()->value());
  if (entry->type == SymbolTable::EXTERNAL) {
    error_db_->AddError(ErrorDB::ERROR, "cannot define external imported symbol",
                        node->label()->pos());
    success_ = false;
    return;
  }
  if (entry->type == SymbolTable::INTERNAL) {
    if (entry->defined) {
      error_db_->AddError(ErrorDB::ERROR, "duplicate symbol", node->label()->pos());
      success_ = false;
      return;
    }
    if (entry->exported && !relative) {
      error_db_->AddError(ErrorDB::ERROR,
                          "symbol marked as exported must be set to a relative value",
                          node->label()->pos());
      success_ = false;
      return;
    }
  }
  if (entry->type == SymbolTable::UNKNOWN) {
    entry->type = SymbolTable::INTERNAL;
  }
  entry->relative = relative;
  entry->defined = true;
  entry->block = -1;
  entry->resolved = true;
  entry->address = value;
  entry->block_address = value;
}

void TableBuilder::VisitNode(node::DirectiveUse* node) {
  EndSegment();
  BlockTable::Entry* entry = nullptr;
  if (node->block_name() == nullptr) {
    entry = block_table_->GetBlock(0);
  } else {
    entry = block_table_->FindNameOrCreate(node->block_name()->value());
  }
  node->set_block_id(entry->block);
  current_block_ = entry->block;
  current_address_ = entry->size;
}

void TableBuilder::VisitNode(node::DirectiveLtorg*) {
  InsertLiterals(true);
}

void TableBuilder::VisitNode(node::DirectiveBase* node) {
  ReferenceExpression(node->expression());
}

void TableBuilder::VisitNode(node::DirectiveNobase*) {}

void TableBuilder::VisitNode(node::DirectiveExtdef* node) {
  for (const auto& token : node->symbol_list()) {
    if (!ExportSymbol(*token)) {
      success_ = false;
      return;
    }
  }
}

void TableBuilder::VisitNode(node::DirectiveExtref* node) {
  for (const auto& token : node->symbol_list()) {
    if (!ImportSymbol(*token)) {
      success_ = false;
      return;
    }
  }
}

void TableBuilder::VisitNode(node::DirectiveMemInit* node) {
  if (node->label() != nullptr) {
    if (!DefineSymbol(*node->label())) {
      success_ = false;
      return;
    }
  }
  if (node->data_token() == nullptr) {
    ReferenceExpression(node->expression());
    current_address_ += (node->width() == node::DirectiveMemInit::WORD) ? 3 : 1;
  } else {
    uint32 size = node->data_token()->data().size();
    if (node->width() == node::DirectiveMemInit::WORD) {
      size = ((size + 2) / 3) * 3;  // round up to multiple of 3
    }
    current_address_ += size;
  }
}

void TableBuilder::VisitNode(node::DirectiveMemReserve* node) {
  if (node->label() != nullptr) {
    if (!DefineSymbol(*node->label())) {
      success_ = false;
      return;
    }
  }
  int32 size = 0;
  if (!SolveAbsoluteExpression(node->expression(), 1, 0xfffff, &size)) {
    success_ = false;
    return;
  }
  node->set_reservation_size(size);
  if (node->width() == node::DirectiveMemReserve::WORD) {
    size *= 3;
  }
  current_address_ += size;
}

void TableBuilder::VisitNode(node::DirectiveInternalLiteral*) {
  assert(false);
}

void TableBuilder::InsertLiterals(bool after_node) {
  literal_table_->ClearLookupTables();
  int literal_table_size = literal_table_->entries().size();
  for (; next_literal_id_ < literal_table_size; next_literal_id_++) {
    // insert node
    unique_ptr<node::DirectiveInternalLiteral> node(new node::DirectiveInternalLiteral);
    node->set_literal_id(next_literal_id_);
    if (after_node) {
      // insert after current node and advance it_
      auto next_it = it_;
      ++next_it;
      code_->mutable_nodes()->emplace(next_it, std::move(node));
      ++it_;
    } else {
      // insert before current node
      code_->mutable_nodes()->emplace(it_, std::move(node));
    }
    // create symbol
    string symbol_name = LiteralTable::LiteralSymbolName(next_literal_id_);
    SymbolTable::Entry* symbol_entry = symbol_table_->FindOrCreateNew(symbol_name);
    assert(symbol_entry->type == SymbolTable::UNKNOWN);
    symbol_entry->type = SymbolTable::INTERNAL;
    symbol_entry->relative = true;
    symbol_entry->defined = true;
    symbol_entry->block = current_block_;
    symbol_entry->block_address = current_address_;
    if (current_block_ == -1) {
      symbol_entry->address = current_address_;
      symbol_entry->resolved = true;
    } else if (current_block_ == 0) {
      symbol_entry->address = start_address_ + current_address_;
      symbol_entry->resolved = true;
    }
    // advance current_address_
    const LiteralTable::Entry* entry = literal_table_->GetLiteral(next_literal_id_);
    switch (entry->type) {
      case LiteralTable::BYTE:
        current_address_ += 1;
        break;
      case LiteralTable::WORD:
        current_address_ += 3;
        break;
      case LiteralTable::FLOAT:
        current_address_ += 6;
        break;
    }
  }
}

bool TableBuilder::SolveAbsoluteExpression(const node::Node::TokenList& expression,
                                           int32 limit_min, int32 limit_max,
                                           int32* result) {
  int32 value = 0;
  bool relative = false;
  if (!ExpressionUtil::Solve(expression, *symbol_table_, false,
                             &value, &relative, nullptr, error_db_)) {
    return false;
  }
  if (relative) {
    error_db_->AddError(ErrorDB::ERROR, "expected absolute expression",
                        expression.front()->pos(), expression.back()->pos());
    return false;
  }
  if (!CheckExpressionLimits(limit_min, limit_max, value, expression)) {
    return false;
  }
  *result = value;
  return true;
}

bool TableBuilder::SolveRelativeExpression(const node::Node::TokenList& expression,
                                          int32 limit_min, int32 limit_max,
                                          int32* result) {
  int32 value = 0;
  bool relative = false;
  if (!ExpressionUtil::Solve(expression, *symbol_table_, false,
                             &value, &relative, nullptr, error_db_)) {
    return false;
  }
  if (!relative) {
    error_db_->AddError(ErrorDB::ERROR, "expected relative expression",
                        expression.front()->pos(), expression.back()->pos());
    return false;
  }
  if (!CheckExpressionLimits(limit_min, limit_max, value, expression)) {
    return false;
  }
  *result = value;
  return true;
}

void TableBuilder::EndSegment() {
  if (current_block_ == -1) {
    segments_.push_back(make_pair(segment_start_, current_address_));
  } else {
    BlockTable::Entry* entry = block_table_->GetBlock(current_block_);
    entry->size = current_address_;
  }
}

bool TableBuilder::CheckAddressOverflow(uint32 address) {
  if (address > 0x100000) {
    error_db_->AddError(ErrorDB::ERROR, "address overflow");
    return false;
  }
  return true;
}

bool TableBuilder::CheckExpressionLimits(int32 limit_min, int32 limit_max, int32 value,
                                         const node::Node::TokenList& expression) {
  if (value < limit_min) {
    char message_buffer[200];
    snprintf(message_buffer, sizeof(message_buffer),
             "expression value must be >= %d", limit_min);
    error_db_->AddError(ErrorDB::ERROR, message_buffer,
                        expression.front()->pos(), expression.back()->pos());
    return false;
  } else if (value > limit_max) {
    char message_buffer[200];
    snprintf(message_buffer, sizeof(message_buffer),
             "expression value must be <= %d", limit_max);
    error_db_->AddError(ErrorDB::ERROR, message_buffer,
                        expression.front()->pos(), expression.back()->pos());
    return false;
  }
  return true;
}

bool TableBuilder::DefineSymbol(const Token& token) {
  SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(token.value());
  if (entry->type == SymbolTable::EXTERNAL) {
    error_db_->AddError(ErrorDB::ERROR, "cannot define external imported symbol",
                        token.pos());
    return false;
  }
  if (entry->type == SymbolTable::INTERNAL && entry->defined) {
    error_db_->AddError(ErrorDB::ERROR, "duplicate symbol", token.pos());
    return false;
  }
  if (entry->type == SymbolTable::UNKNOWN) {
    entry->type = SymbolTable::INTERNAL;
  }
  entry->relative = true;
  entry->defined = true;
  entry->block = current_block_;
  entry->block_address = current_address_;
  if (current_block_ == -1) {
    entry->address = current_address_;
    entry->resolved = true;
  } else if (current_block_ == 0) {
    entry->address = start_address_ + current_address_;
    entry->resolved = true;
  }
  return true;
}

void TableBuilder::ReferenceSymbol(const std::string& name) {
  SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(name);
  entry->referenced = true;
}

void TableBuilder::ReferenceSymbolToken(const Token& token) {
  SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(token.value());
  if (entry->type == SymbolTable::UNKNOWN ||
      (entry->type == SymbolTable::INTERNAL && !entry->defined)) {
    undefined_symbol_tokens_.push_back(make_pair(&token, entry));
  }
  entry->referenced = true;
}

void TableBuilder::ReferenceExpression(const node::Node::TokenList& expression) {
  for (const auto& token : expression) {
    if (token->type() == Token::NAME) {
      ReferenceSymbolToken(*token);
    }
  }
}

bool TableBuilder::ImportSymbol(const Token& token) {
  SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(token.value());
  if (entry->type == SymbolTable::EXTERNAL) {
    error_db_->AddError(ErrorDB::WARNING, "symbol already marked as imported",
                        token.pos());
    return true;
  }
  if (entry->type == SymbolTable::INTERNAL) {
    if (entry->defined) {
      error_db_->AddError(ErrorDB::ERROR, "cannot import symbol which is already defined",
                          token.pos());
    } else {
      assert(entry->exported);
      error_db_->AddError(ErrorDB::ERROR, "cannot import symbol marked as exported",
                          token.pos());
    }
    return false;
  }
  entry->type = SymbolTable::EXTERNAL;
  entry->relative = true;
  return true;
}

bool TableBuilder::ExportSymbol(const Token& token) {
  SymbolTable::Entry* entry = symbol_table_->FindOrCreateNew(token.value());
  if (entry->type == SymbolTable::EXTERNAL) {
    error_db_->AddError(ErrorDB::ERROR, "cannot export symbol marked as imported",
                        token.pos());
    return false;
  }
  if (entry->type == SymbolTable::UNKNOWN) {
    entry->type = SymbolTable::INTERNAL;
    entry->relative = true;
  }
  if (!entry->defined) {
    undefined_symbol_tokens_.push_back(make_pair(&token, entry));
  }
  if (entry->exported) {
    error_db_->AddError(ErrorDB::WARNING, "symbol already marked as exported",
                        token.pos());
    return true;
  }
  if (entry->defined && !entry->relative) {
    error_db_->AddError(ErrorDB::ERROR, "cannot export absolute symbols", token.pos());
    return false;
  }
  entry->exported = true;
  entry->referenced = true;
  return true;
}

bool TableBuilder::OrderBlocks() {
  uint32 start = 0, end = 0;
  block_table_->OrderBlocks(&start, &end);
  if (!CheckAddressOverflow(end)) {
    return false;
  }
  segments_.push_back(make_pair(start, end));
  return true;
}

bool TableBuilder::CheckIntersect() {
  vector<pair<uint32, int> > events;
  for (const auto& segment : segments_) {
    events.push_back(make_pair(segment.first, 0));  // segment start event
    events.push_back(make_pair(segment.second, 1));  // segment end event
  }
  sort(events.begin(), events.end());

  vector<pair<uint32, uint32> > intersections;
  int count = 0;
  uint32 intersect_start = 0;
  for (const auto& event : events) {
    if (event.second == 1) {
      count--;
      if (count == 1 && intersect_start != event.first) {
        intersections.push_back(make_pair(intersect_start, event.first));
      }
    } else {
      if (count == 1) {
        intersect_start = event.first;
      }
      count++;
    }
  }
  if (intersections.empty()) {
    return true;
  }
  for (const auto& intersection : intersections) {
    char message_buffer[200];
    snprintf(message_buffer, sizeof(message_buffer),
             "code intersects from address 0x%06x to address 0x%06x",
             intersection.first, intersection.second);
    error_db_->AddError(ErrorDB::ERROR, message_buffer);
  }
  return false;
}

void TableBuilder::FinalizeSymbolTable(bool* has_undefined_symbols) {
  *has_undefined_symbols = false;
  for (auto& entry_pair : *symbol_table_->mutable_entry_map()) {
    SymbolTable::Entry* entry = &entry_pair.second;
    // set final address for symbols from blocks
    if (entry->type == SymbolTable::INTERNAL &&
        entry->defined && !entry->resolved) {
      assert(entry->block > 0);
      const BlockTable::Entry* block_entry = block_table_->GetBlock(entry->block);
      entry->address = block_entry->start + entry->block_address;
      entry->resolved = true;
    }
    // check if undefined
    if (entry->referenced &&
        (entry->type == SymbolTable::UNKNOWN ||
         (entry->type == SymbolTable::INTERNAL && !entry->defined))) {
      *has_undefined_symbols = true;
    }
  }
}

void TableBuilder::UndefinedSymbolsError() {
  size_t count = 0;
  for (const auto& token_entry_pair : undefined_symbol_tokens_) {
    const Token* token = token_entry_pair.first;
    const SymbolTable::Entry* entry = token_entry_pair.second;
    if (entry->referenced &&
        (entry->type == SymbolTable::UNKNOWN ||
         (entry->type == SymbolTable::INTERNAL && !entry->defined))) {
      error_db_->AddError(ErrorDB::ERROR, "undefined symbol", token->pos());
      count++;
    }
  }
  assert(count > 0);
}

uint32 TableBuilder::GetEndAddress() {
  uint32 end_address = 0;
  for (const auto& segment : segments_) {
    if (segment.second > end_address) {
      end_address = segment.second;
    }
  }
  return end_address;
}

}  // namespace assembler
}  // namespace sicxe
