#include "assembler/log_file_writer.h"

#include <algorithm>
#include "assembler/block_table.h"
#include "assembler/code.h"
#include "assembler/literal_table.h"
#include "assembler/symbol_table.h"
#include "common/cpu_state.h"
#include "common/instruction.h"
#include "common/instruction_db.h"
#include "common/text_file.h"

using std::string;

namespace sicxe {
namespace assembler {

static const size_t kBufferSize = 1024;
static const char* kCodeHeader =
    "********** CODE ********************************************";
static const char* kBlockTableHeader =
    "********** BLOCKS ******************************************";
static const char* kLiteralTableHeader =
    "********** LITERALS ****************************************";
static const char* kSymbolTableHeader =
    "********** SYMBOLS *****************************************";

static const size_t kColumnLabel = 21;
static const size_t kColumnMnemonic = 35;
static const size_t kColumnComment = 55;
static const size_t kColumnCommentEmptyNode = 21;
static const size_t kCommentMinSpace = 2;
static const size_t kCommentRoundTo = 5;
static const size_t kColumnOperands = 45;

LogFileWriter::LogFileWriter(const InstructionDB* instruction_db, TextFile* text_file)
    : instruction_db_(instruction_db), text_file_(text_file), code_(nullptr),
      buffer_(new char[kBufferSize]), has_data_(false), data_(nullptr), address_(0) {}

LogFileWriter::~LogFileWriter() {}

void LogFileWriter::Begin(const Code& code) {
  code_ = &code;
  text_file_->mutable_lines()->emplace_back(kCodeHeader);
}

void LogFileWriter::WriteNode(const node::Node& node, uint32 address) {
  address_ = address;
  has_data_ = false;
  node.AcceptVisitor(this);
  text_file_->mutable_lines()->emplace_back(std::move(line_));
  line_ = string();
}

void LogFileWriter::WriteNode(const node::Node& node, uint32 address,
                              const std::string& data, bool) {
  address_ = address;
  has_data_ = true;
  data_ = &data;
  node.AcceptVisitor(this);
  text_file_->mutable_lines()->emplace_back(std::move(line_));
  line_ = string();
}

void LogFileWriter::WriteRelocationRecord(const RelocationRecord&) {}

void LogFileWriter::End() {
  text_file_->mutable_lines()->emplace_back(kBlockTableHeader);
  code_->block_table()->OutputToTextFile(text_file_);
  if (!code_->literal_table()->entries().empty()) {
    text_file_->mutable_lines()->emplace_back(kLiteralTableHeader);
    code_->literal_table()->OutputToTextFile(text_file_);
  }
  text_file_->mutable_lines()->emplace_back(kSymbolTableHeader);
  code_->symbol_table()->OutputToTextFile(code_->block_table(), text_file_);
}

void LogFileWriter::VisitNode(const node::Empty* node) {
  LineWriteAddress();
  if (node->comment() != nullptr) {
    AddSpaces(kColumnCommentEmptyNode);
    line_ += node->comment()->value();
  }
}

void LogFileWriter::VisitNode(const node::InstructionF1* node) {
  LineWriteInstruction(node, false);
  LineWriteComment(node);
}

namespace {

string* CreateIndexRegName() {
  string* name = new string;
  bool success = CpuState::RegisterIdToName(CpuState::REG_X, name);
  assert(success);
  unused(success);
  return name;
}

}  // namespace

void LogFileWriter::VisitNode(const node::InstructionF2* node) {
  LineWriteInstruction(node, false);
  AddSpaces(kColumnOperands);
  string reg_name;
  bool s1 = CpuState::RegisterIdToName(
      static_cast<CpuState::RegisterId>(node->r1()), &reg_name);
  assert(s1);
  unused(s1);
  line_ += reg_name;
  if (node->syntax() == Syntax::F2_REG_REG) {
    line_ += ", ";
    bool s2 = CpuState::RegisterIdToName(
        static_cast<CpuState::RegisterId>(node->r2()), &reg_name);
    assert(s2);
    unused(s2);
    line_ += reg_name;
  } else if (node->syntax() == Syntax::F2_REG_N) {
    snprintf(buffer_.get(), kBufferSize, ", %u", static_cast<uint32>(node->r2()));
    line_ += string(buffer_.get());
  }
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::InstructionFS34* node) {
  LineWriteInstruction(node, node->extended());
  if (node->syntax() != Syntax::FS34_NONE) {
    AddSpaces(kColumnOperands);
    if (node->addressing() == node::InstructionFS34::LITERAL_POOL) {
      line_ += LiteralTable::LiteralSymbolName(node->literal_id());
    } else {
      bool brackets = false;
      if (node->addressing() == node::InstructionFS34::IMMEDIATE) {
        line_ += "#";
        brackets = true;
      }
      if (node->addressing() == node::InstructionFS34::INDIRECT) {
        line_ += "@";
        brackets = true;
      }
      brackets = brackets && (node->expression().size() > 1);
      if (brackets) {
        line_ += "[";
      }
      LineWriteExpression(node->expression());
      if (brackets) {
        line_ += "]";
      }
    }

    if (node->indexed()) {
      static string* index_reg_name = CreateIndexRegName();
      line_ += ", ";
      line_ += *index_reg_name;
    }
  }
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveStart* node) {
  LineWriteDirective(node);
  AddSpaces(kColumnOperands);
  LineWriteExpression(node->expression());
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveEnd* node) {
  LineWriteDirective(node);
  AddSpaces(kColumnOperands);
  LineWriteExpression(node->expression());
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveOrg* node) {
  LineWriteDirective(node);
  AddSpaces(kColumnOperands);
  LineWriteExpression(node->expression());
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveEqu* node) {
  LineWriteDirective(node);
  AddSpaces(kColumnOperands);
  if (node->assign_current_address()) {
    line_ += "*";
  } else {
    LineWriteExpression(node->expression());
  }
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveUse* node) {
  LineWriteDirective(node);
  AddSpaces(kColumnOperands);
  if (node->block_name() != nullptr) {
    line_ += node->block_name()->value();
  }
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveLtorg* node) {
  LineWriteDirective(node);
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveBase* node) {
  LineWriteDirective(node);
  AddSpaces(kColumnOperands);
  LineWriteExpression(node->expression());
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveNobase* node) {
  LineWriteDirective(node);
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveExtdef* node) {
  LineWriteDirective(node);
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveExtref* node) {
  LineWriteDirective(node);
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveMemInit* node) {
  LineWriteDirective(node);
  AddSpaces(kColumnOperands);
  if (node->data_token() == nullptr) {
    LineWriteExpression(node->expression());
  } else {
    LineWriteDataLiteral(node->data_token()->data());
  }
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveMemReserve* node) {
  LineWriteDirective(node);
  AddSpaces(kColumnOperands);
  LineWriteExpression(node->expression());
  LineWriteComment(node);
}

void LogFileWriter::VisitNode(const node::DirectiveInternalLiteral* node) {
  LineWriteAddress();
  LineWriteData();
  LineWriteLabel(LiteralTable::LiteralSymbolName(node->literal_id()));
  AddSpaces(kColumnMnemonic);
  if (data_->size() > 1) {
    line_ += "WORD";
  } else {
    line_ += "BYTE";
  }
  AddSpaces(kColumnOperands);
  LineWriteDataLiteral(*data_);
}

void LogFileWriter::AddSpaces(size_t columns) {
  if (columns > line_.size()) {
    line_.resize(columns, ' ');
  }
}

void LogFileWriter::LineWriteAddress() {
  snprintf(buffer_.get(), kBufferSize, "%06x:", address_);
  line_ += string(buffer_.get());
}

void LogFileWriter::LineWriteData() {
  if (!has_data_) {
    return;
  }
  if (data_->size() <= 4) {
    for (size_t i = 0; i < data_->size(); i++) {
      snprintf(buffer_.get(), kBufferSize, " %02x",
               static_cast<uint32>(data_->at(i)) & 0xff);
      line_ += string(buffer_.get());
    }
  } else {
    snprintf(buffer_.get(), kBufferSize, " %02x ..... %02x",
             static_cast<uint32>(data_->front()) & 0xff,
             static_cast<uint32>(data_->back()) & 0xff);
    line_ += string(buffer_.get());
  }
}

void LogFileWriter::LineWriteLabel(const std::string& label) {
  AddSpaces(kColumnLabel);
  size_t label_max_size = kColumnMnemonic - kColumnLabel - 2;
  if (label.size() > label_max_size) {
    string label_short = label;
    label_short.resize(label_max_size - 1, 0x00);
    label_short += "~";
    line_ += label_short;
  } else {
    line_ += label;
  }
}

void LogFileWriter::LineWriteInstruction(const node::Instruction* node, bool extended) {
  LineWriteAddress();
  LineWriteData();
  if (node->label() != nullptr) {
    LineWriteLabel(node->label()->value());
  }
  AddSpaces(kColumnMnemonic);
  string mnemonic;
  if (extended) {
    mnemonic = "+";
  }
  mnemonic += instruction_db_->FindOpcode(node->opcode())->mnemonic();
  size_t mnemonic_max_size = kColumnOperands - kColumnMnemonic - 2;
  if (mnemonic.size() > mnemonic_max_size) {
    mnemonic.resize(mnemonic_max_size - 1, 0x00);
    mnemonic += "~";
  }
  line_ += mnemonic;
}

void LogFileWriter::LineWriteDirective(const node::Directive* node) {
  LineWriteAddress();
  LineWriteData();
  if (node->label() != nullptr) {
    LineWriteLabel(node->label()->value());
  }
  AddSpaces(kColumnMnemonic);
  const char* mnemonic_str = nullptr;
  switch (node->directive_id()) {
    case Directive::START:
      mnemonic_str = "START";
      break;
    case Directive::END:
      mnemonic_str = "END";
      break;
    case Directive::ORG:
      mnemonic_str = "ORG";
      break;
    case Directive::EQU:
      mnemonic_str = "EQU";
      break;
    case Directive::USE:
      mnemonic_str = "USE";
      break;
    case Directive::LTORG:
      mnemonic_str = "LTORG";
      break;
    case Directive::BASE:
      mnemonic_str = "BASE";
      break;
    case Directive::NOBASE:
      mnemonic_str = "NOBASE";
      break;
    case Directive::EXTDEF:
      mnemonic_str = "EXTDEF";
      break;
    case Directive::EXTREF:
      mnemonic_str = "EXTREF";
      break;
    case Directive::BYTE:
      mnemonic_str = "BYTE";
      break;
    case Directive::WORD:
      mnemonic_str = "WORD";
      break;
    case Directive::RESB:
      mnemonic_str = "REB";
      break;
    case Directive::RESW:
      mnemonic_str = "RESW";
      break;
    default:
      mnemonic_str = "?";
      break;
  }
  line_ += mnemonic_str;
}

void LogFileWriter::LineWriteExpression(const node::Node::TokenList& expression) {
  bool first = true;
  for (const auto& token : expression) {
    if (first) {
      first = false;
    } else {
      line_ += " ";
    }
    line_ += token->value();
  }
}

void LogFileWriter::LineWriteSymbolList(const node::Node::TokenList& symbols) {
  bool first = true;
  for (const auto& token : symbols) {
    if (first) {
      first = false;
    } else {
      line_ += ", ";
    }
    line_ += token->value();
  }
}

void LogFileWriter::LineWriteDataLiteral(const std::string& data) {
  line_ += "X'";
  size_t size = data.size();
  if (size > 6) {
    size = 6;
  }
  for (size_t i = 0; i < size; i++) {
    snprintf(buffer_.get(), kBufferSize, "%02x", static_cast<uint32>(data[i]) & 0xff);
    line_ += string(buffer_.get());
  }
  if (data.size() > 6) {
    line_ += "...";
  } else {
    line_ += "'";
  }
}

void LogFileWriter::LineWriteComment(const node::Node* node) {
  if (node->comment() != nullptr) {
    size_t column = std::max(line_.size() + kCommentMinSpace, kColumnComment);
    column = ((column + kCommentRoundTo - 1) / kCommentRoundTo) * kCommentRoundTo;
    AddSpaces(column);
    line_ += node->comment()->value();
  }
}

}  // namespace assembler
}  // namespace sicxe
