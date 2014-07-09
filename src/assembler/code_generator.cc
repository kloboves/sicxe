#include "assembler/code_generator.h"

#include "assembler/block_table.h"
#include "assembler/code.h"
#include "assembler/expression_util.h"
#include "assembler/literal_table.h"
#include "assembler/symbol_table.h"
#include "assembler/token.h"
#include "common/error_db.h"
#include "common/format.h"
#include "common/format_util.h"
#include "common/instruction_instance.h"

using std::string;

namespace sicxe {
namespace assembler {

RelocationRecord::RelocationRecord(uint32 addr, int n)
    : type(SIMPLE), address(addr), nibbles(n), sign(false) {}

RelocationRecord::RelocationRecord(uint32 addr, int n, bool sgn, const std::string& name)
    : type(SYMBOL), address(addr), nibbles(n), sign(sgn), symbol_name(name) {}

RelocationRecord::~RelocationRecord() {}

CodeGenerator::CodeGenerator()
    : code_(nullptr), symbol_table_(nullptr), block_table_(nullptr),
      literal_table_(nullptr), output_writers_(nullptr), error_db_(nullptr),
      current_block_(-1), current_address_(0), base_enabled_(false),
      base_relative_(false), base_address_(0), success_(false) {}

CodeGenerator::~CodeGenerator() {}

bool CodeGenerator::GenerateCode(const Code& code, OutputWriterVector* output_writers,
                                 ErrorDB* error_db) {
  code_ = &code;
  symbol_table_ = code_->symbol_table();
  block_table_ = code_->block_table();
  literal_table_ = code_->literal_table();
  output_writers_ = output_writers;
  error_db_ = error_db;
  current_block_ = 0;
  current_address_ = 0;
  base_enabled_ = false;
  base_relative_ = false;
  base_address_ = 0;
  success_ = true;

  block_address_table_.reset(new uint32[block_table_->entries().size()]);
  for (const auto& entry : block_table_->entries()) {
    block_address_table_[entry->block] = entry->start;
  }

  Begin();
  for (const auto& node : code_->nodes()) {
    node->AcceptVisitor(this);
    if (!success_) {
      return false;
    }
  }
  End();
  block_address_table_.reset();
  return true;
}

void CodeGenerator::VisitNode(const node::Empty* node) {
  WriteNode(*node);
}

namespace {

string EncodeInstance(const InstructionInstance& instance, int size) {
  uint8 raw_data[4];
  int encoded_length = 0;
  bool success = FormatUtil::Encode(instance, raw_data, &encoded_length);
  assert(success);
  unused(success);
  assert(size == encoded_length);
  string data(size, 0x00);
  for (int i = 0; i < size; i++) {
    data[i] = raw_data[i];
  }
  return data;
}

}  // namespace

void CodeGenerator::VisitNode(const node::InstructionF1* node) {
  const int size = 1;
  InstructionInstance instance;
  instance.opcode = node->opcode();
  instance.format = node->format();
  WriteNode(*node, EncodeInstance(instance, size), true);
  current_address_ += size;
}

void CodeGenerator::VisitNode(const node::InstructionF2* node) {
  const int size = 2;
  InstructionInstance instance;
  instance.opcode = node->opcode();
  instance.format = node->format();
  instance.operands.f2.r1 = node->r1();
  instance.operands.f2.r2 = node->r2();
  WriteNode(*node, EncodeInstance(instance, size), true);
  current_address_ += size;
}

bool CodeGenerator::TryPCRelativeAddressing(int32 value, bool relative,
                                            InstructionInstance* instance) {
  if (!relative) {
    return false;
  }
  int32 next_address = current_address_ + 3;
  int32 offset = value - next_address;
  bool in_range = (offset >= -2048 && offset <= 2047);
  if (!in_range) {
    return false;
  }
  instance->operands.fS34.p = true;
  instance->operands.fS34.b = false;
  instance->operands.fS34.address = static_cast<uint32>(offset) & 0xfff;
  return true;
}

bool CodeGenerator::TryBaseAddressing(int32 value, bool relative,
                                      InstructionInstance* instance) {
  if (!base_enabled_ || (relative != base_relative_)) {
    return false;
  }
  int32 offset = value - static_cast<int>(base_address_);
  bool in_range = (offset >= 0 && offset <= 4095);
  if (!in_range) {
    return false;
  }
  instance->operands.fS34.p = false;
  instance->operands.fS34.b = true;
  instance->operands.fS34.address = static_cast<uint32>(offset) & 0xfff;
  return true;
}

void CodeGenerator::VisitNode(const node::InstructionFS34* node) {
  const int size = node->extended() ? 4 : 3;
  bool can_split = true;
  InstructionInstance instance;
  instance.opcode = node->opcode();
  instance.format = node->format();
  if (node->syntax() == Syntax::FS34_NONE) {
    if (node->extended()) {
      instance.operands.fS34.n = true;
      instance.operands.fS34.i = true;
    } else {
      instance.operands.fS34.n = false;
      instance.operands.fS34.i = false;
    }
    instance.operands.fS34.e = node->extended();
    instance.operands.fS34.x = false;
    instance.operands.fS34.p = false;
    instance.operands.fS34.b = false;
    instance.operands.fS34.address = 0;
  } else {
    instance.operands.fS34.e = node->extended();
    instance.operands.fS34.x = node->indexed();
    switch (node->addressing()) {
      case node::InstructionFS34::IMMEDIATE:
        instance.operands.fS34.n = false;
        instance.operands.fS34.i = true;
        break;
      case node::InstructionFS34::INDIRECT:
        instance.operands.fS34.n = true;
        instance.operands.fS34.i = false;
        break;
      case node::InstructionFS34::SIMPLE:
      case node::InstructionFS34::LITERAL_POOL:
        instance.operands.fS34.n = true;
        instance.operands.fS34.i = true;
        break;
    }

    int32 value = 0;
    bool relative = false;
    ExpressionUtil::ExternalSymbolVector external_symbols;
    if (node->addressing() == node::InstructionFS34::LITERAL_POOL) {
      const SymbolTable::Entry* entry =
          symbol_table_->Find(LiteralTable::LiteralSymbolName(node->literal_id()));
      assert(entry->type == SymbolTable::INTERNAL);
      value = entry->address;
      relative = entry->relative;
    } else {
      if (!ExpressionUtil::Solve(node->expression(), *symbol_table_, true,
                                 &value, &relative, &external_symbols, error_db_) ||
          !CheckExpressionLimits(0, 0xffffff, value, node->expression())) {
        success_ = false;
        return;
      }
    }

    bool addressing_selected = false;
    if (external_symbols.empty() && !node->extended()) {
      if (TryPCRelativeAddressing(value, relative, &instance)) {
        // PC relative addressing selected
        addressing_selected = true;
      } else if (TryBaseAddressing(value, relative, &instance)) {
        // base addressing selected
        addressing_selected = true;
      }
    }
    if (!addressing_selected) {
      // direct addressing selected
      bool in_range = false;
      if (node->extended()) {
        in_range = value <= 1048575;
      } else {
        in_range = value <= 4095;
      }
      if (in_range) {
        // direct addressing possible
        uint32 relocation_address = current_address_ + 1;
        int nibbles = node->extended() ? 5 : 3;
        if (!external_symbols.empty()) {
          for (const auto& symbol : external_symbols) {
            RelocationRecord record(relocation_address, nibbles,
                                    symbol.sign == ExpressionUtil::PLUS, symbol.name);
            WriteRelocationRecord(record);
          }
          can_split = false;
        } else if (relative) {
          WriteRelocationRecord(RelocationRecord(relocation_address, nibbles));
          can_split = false;
        }
        instance.operands.fS34.p = false;
        instance.operands.fS34.b = false;
        if (node->extended()) {
          instance.operands.fS34.address = static_cast<uint32>(value) & 0xfffff;
        } else {
          instance.operands.fS34.address = static_cast<uint32>(value) & 0xfff;
        }
      } else {
        // direct addressing impossible, try to resort to SIC addressing otherwise fail
        if ((node->addressing() == node::InstructionFS34::SIMPLE ||
             node->addressing() == node::InstructionFS34::LITERAL_POOL) &&
            value <= 32767 && external_symbols.empty() && !relative) {
          // SIC addressing possible
          instance.operands.fS34.n = false;
          instance.operands.fS34.i = false;
          instance.operands.fS34.p = false;
          instance.operands.fS34.b = false;
          instance.operands.fS34.address = static_cast<uint32>(value) & 0x7fff;
        } else {
          // no addressing possible, fail
          const char* message = nullptr;
          if (node->addressing() == node::InstructionFS34::LITERAL_POOL) {
            message = "could not address literal";
          } else {
            message = "could not address operand";
          }
          error_db_->AddError(ErrorDB::ERROR, message, node->expression().front()->pos(),
                              node->expression().back()->pos());
          success_ = false;
          return;
        }
      }
    }
  }
  WriteNode(*node, EncodeInstance(instance, size), can_split);
  current_address_ += size;
}

void CodeGenerator::VisitNode(const node::DirectiveStart* node) {
  current_block_ = 0;
  current_address_ = block_address_table_[0];
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveEnd* node) {
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveOrg* node) {
  if (current_block_ >= 0) {
    block_address_table_[current_block_] = current_address_;
  }
  current_block_ = -1;
  current_address_ = node->address();
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveEqu* node) {
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveUse* node) {
  if (current_block_ >= 0) {
    block_address_table_[current_block_] = current_address_;
  }
  current_block_ = node->block_id();
  current_address_ = block_address_table_[current_block_];
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveLtorg* node) {
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveBase* node) {
  int32 value = 0;
  bool relative = false;
  if (!ExpressionUtil::Solve(node->expression(), *symbol_table_, false,
                             &value, &relative, nullptr, error_db_) ||
      !CheckExpressionLimits(0, 0xfffff, value, node->expression())) {
    success_ = false;
    return;
  }
  base_enabled_ = true;
  base_relative_ = relative;
  base_address_ = value;
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveNobase* node) {
  base_enabled_ = false;
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveExtdef* node) {
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveExtref* node) {
  WriteNode(*node);
}

void CodeGenerator::VisitNode(const node::DirectiveMemInit* node) {
  if (node->data_token() != nullptr) {
    WriteNode(*node, node->data_token()->data(), true);
    if (node->width() == node::DirectiveMemInit::WORD) {
      uint32 size = node->data_token()->data().size();
      size = ((size + 2) / 3) * 3;  // round up to multiple of 3
      current_address_ += size;
    } else {
      current_address_ += node->data_token()->data().size();
    }
  } else {
    int32 value = 0;
    bool relative = false;
    ExpressionUtil::ExternalSymbolVector external_symbols;
    const bool is_byte = (node->width() == node::DirectiveMemInit::BYTE);
    int32 limit_min = is_byte ? -(1 << 7) : -(1 << 23);
    int32 limit_max = is_byte ? (1 << 8) - 1 : (1 << 24) - 1;
    if (!ExpressionUtil::Solve(node->expression(), *symbol_table_, true,
                               &value, &relative, &external_symbols, error_db_) ||
        !CheckExpressionLimits(limit_min, limit_max, value, node->expression())) {
      success_ = false;
      return;
    }
    value &= limit_max;
    int size = (node->width() == node::DirectiveMemInit::WORD) ? 3 : 1;
    bool can_split = true;
    if (!external_symbols.empty()) {
      for (const auto& symbol : external_symbols) {
        RelocationRecord record(current_address_, 2 * size,
                                symbol.sign == ExpressionUtil::PLUS, symbol.name);
        WriteRelocationRecord(record);
      }
      can_split = false;
    } else if (relative) {
      WriteRelocationRecord(RelocationRecord(current_address_, 2 * size));
      can_split = false;
    }
    string data(size, 0x00);
    for (int i = size -1; i >= 0; i--) {
      data[i] = value & 0xff;
      value >>= 8;
    }
    WriteNode(*node, data, can_split);
    current_address_ += size;
  }
}

void CodeGenerator::VisitNode(const node::DirectiveMemReserve* node) {
  WriteNode(*node);
  if (node->width() == node::DirectiveMemReserve::WORD) {
    current_address_ += 3 * node->reservation_size();
  } else {
    current_address_ += node->reservation_size();
  }
}

void CodeGenerator::VisitNode(const node::DirectiveInternalLiteral* node) {
  const LiteralTable::Entry* entry = literal_table_->GetLiteral(node->literal_id());
  int size = 0;
  uint64 value = 0;
  switch (entry->type) {
    case LiteralTable::BYTE:
      size = 1;
      value = entry->value.byte;
      break;
    case LiteralTable::WORD:
      size = 3;
      value = entry->value.word;
      break;
    case LiteralTable::FLOAT:
      size = 6;
      value = entry->value.float_bin;
      break;
  }
  string data(size, 0x00);
  for (int i = size - 1; i >= 0; i--) {
    data[i] = value & 0xff;
    value >>= 8;
  }
  WriteNode(*node, data, true);
  current_address_ += size;
}

void CodeGenerator::Begin() {
  for (CodeOutputWriter* writer : *output_writers_) {
    writer->Begin(*code_);
  }
}

void CodeGenerator::WriteNode(const node::Node& node) {
  for (CodeOutputWriter* writer : *output_writers_) {
    writer->WriteNode(node, current_address_);
  }
}

void CodeGenerator::WriteNode(const node::Node& node,
                              const std::string& data, bool can_split) {
  for (CodeOutputWriter* writer : *output_writers_) {
    writer->WriteNode(node, current_address_, data, can_split);
  }
}

void CodeGenerator::WriteRelocationRecord(const RelocationRecord& record) {
  for (CodeOutputWriter* writer : *output_writers_) {
    writer->WriteRelocationRecord(record);
  }
}

void CodeGenerator::End() {
  for (CodeOutputWriter* writer : *output_writers_) {
    writer->End();
  }
}

bool CodeGenerator::CheckExpressionLimits(int32 limit_min, int32 limit_max, int32 value,
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

}  // namespace assembler
}  // namespace sicxe
