#include "test_node_visitor.h"

#include <gtest/gtest.h>
#include "assembler/block_table.h"
#include "assembler/literal_table.h"
#include "assembler/symbol_table.h"

using std::string;
using namespace sicxe::assembler;

namespace sicxe {
namespace tests {

void TestNodeVisitor::DumpCode(const assembler::Code& code, TextFile* text_file) {
  text_file->set_file_name("code_dump.txt");
  TestNodeVisitor c;
  c.DumpCodeInternal(code, text_file);
}

void TestNodeVisitor::DumpCodeAndTables(const assembler::Code& code, TextFile* text_file) {
  text_file->set_file_name("code_dump.txt");
  TestNodeVisitor c;
  c.DumpCodeInternal(code, text_file);
  text_file->mutable_lines()->emplace_back("********");
  code.block_table()->OutputToTextFile(text_file);
  text_file->mutable_lines()->emplace_back("********");
  code.literal_table()->OutputToTextFile(text_file);
  text_file->mutable_lines()->emplace_back("********");
  code.symbol_table()->OutputToTextFile(code.block_table(), text_file);
}

TestNodeVisitor::TestNodeVisitor() {}
TestNodeVisitor::~TestNodeVisitor() {}

namespace {

string IntToString(int a) {
  char buffer[20];
  snprintf(buffer, 20, "%d", a);
  return string(buffer);
}

}  // namespace

void TestNodeVisitor::DumpCodeInternal(const assembler::Code& code, TextFile* text_file) {
  text_file->mutable_lines()->emplace_back(
      "File '" + code.text_file()->file_name() + "' code dump:");
  line_ = "program_name='";
  line_ += code.program_name();
  line_ += "' start_address=";
  line_ += IntToString(code.start_address());
  line_ += " end_address=";
  line_ += IntToString(code.end_address());
  line_ += " entry_point=";
  line_ += IntToString(code.entry_point());
  text_file->mutable_lines()->emplace_back(std::move(line_));
  line_ = string();
  text_file->mutable_lines()->emplace_back("********");
  for (const auto& node : code.nodes()) {
    node->AcceptVisitor(this);
    text_file->mutable_lines()->emplace_back(std::move(line_));
    line_ = string();
  }
}

void TestNodeVisitor::VisitNode(const node::Empty* node) {
  unused(node);
  line_ = "Empty";
}

void TestNodeVisitor::VisitNode(const node::InstructionF1* node) {
  OutputInstruction(node);
}

void TestNodeVisitor::VisitNode(const node::InstructionF2* node) {
  OutputInstruction(node);
  line_ += " r1=";
  line_ += IntToString(node->r1());
  line_ += " r2=";
  line_ += IntToString(node->r2());
}

void TestNodeVisitor::VisitNode(const node::InstructionFS34* node) {
  OutputInstruction(node);
  line_ += " addressing=";
  switch (node->addressing()) {
    case node::InstructionFS34::SIMPLE:
      line_ += "simple";
      break;
    case node::InstructionFS34::IMMEDIATE:
      line_ += "immediate";
      break;
    case node::InstructionFS34::INDIRECT:
      line_ += "indirect";
      break;
    case node::InstructionFS34::LITERAL_POOL:
      line_ += "literal_pool";
      break;
  }
  if (node->extended()) {
    line_ += "+extended";
  }
  if (node->indexed()) {
    line_ += "+indexed";
  }
  if (node->addressing() == node::InstructionFS34::LITERAL_POOL) {
    line_ += " literal_id=";
    line_ += IntToString(node->literal_id());
  }
  if (node->data_token() != nullptr) {
    line_ += " data_token=";
    line_ += node->data_token()->value();
  } else {
    OutputExpression(node->expression());
  }
}

void TestNodeVisitor::VisitNode(const node::DirectiveStart* node) {
  OutputDirectiveName("START");
  OutputLabel(node);
  OutputExpression(node->expression());
}

void TestNodeVisitor::VisitNode(const node::DirectiveEnd* node) {
  OutputDirectiveName("END");
  OutputLabel(node);
  OutputExpression(node->expression());
}

void TestNodeVisitor::VisitNode(const node::DirectiveOrg* node) {
  OutputDirectiveName("ORG");
  OutputLabel(node);
  line_ += " address=";
  line_ += IntToString(node->address());
  OutputExpression(node->expression());
}

void TestNodeVisitor::VisitNode(const node::DirectiveEqu* node) {
  OutputDirectiveName("EQU");
  OutputLabel(node);
  if (node->assign_current_address()) {
    line_ += " assign_current=true";
  } else {
    OutputExpression(node->expression());
  }
}

void TestNodeVisitor::VisitNode(const node::DirectiveUse* node) {
  OutputDirectiveName("USE");
  OutputLabel(node);
  line_ += " block_id=";
  line_ += IntToString(node->block_id());
  if (node->block_name() != nullptr) {
    line_ += " block_name=";
    line_ += node->block_name()->value();
  }
}

void TestNodeVisitor::VisitNode(const node::DirectiveLtorg* node) {
  OutputDirectiveName("LTORG");
  OutputLabel(node);
}

void TestNodeVisitor::VisitNode(const node::DirectiveBase* node) {
  OutputDirectiveName("BASE");
  OutputLabel(node);
  OutputExpression(node->expression());
}

void TestNodeVisitor::VisitNode(const node::DirectiveNobase* node) {
  OutputDirectiveName("NOBASE");
  OutputLabel(node);
}

void TestNodeVisitor::VisitNode(const node::DirectiveExtdef* node) {
  OutputDirectiveName("EXTDEF");
  OutputLabel(node);
  OutputSymbolList(node->symbol_list());
}

void TestNodeVisitor::VisitNode(const node::DirectiveExtref* node) {
  OutputDirectiveName("EXTREF");
  OutputLabel(node);
  OutputSymbolList(node->symbol_list());
}

void TestNodeVisitor::VisitNode(const node::DirectiveMemInit* node) {
  OutputDirectiveName("MEMINIT");
  OutputLabel(node);
  line_ += " word=";
  line_ += (node->width() == node::DirectiveMemInit::WORD) ? "true" : "false";
  if (node->data_token() != nullptr) {
    line_ += " data_token=";
    line_ += node->data_token()->value();
  } else {
    OutputExpression(node->expression());
  }
}

void TestNodeVisitor::VisitNode(const node::DirectiveMemReserve* node) {
  OutputDirectiveName("MEMRES");
  OutputLabel(node);
  line_ += " word=";
  line_ += (node->width() == node::DirectiveMemReserve::WORD) ? "true" : "false";
  line_ += " reservation_size=";
  line_ += IntToString(node->reservation_size());
  OutputExpression(node->expression());
}

void TestNodeVisitor::VisitNode(const node::DirectiveInternalLiteral* node) {
  OutputDirectiveName("INTERNAL_LITERAL");
  OutputLabel(node);
  line_ += " literal_id=";
  line_ += IntToString(node->literal_id());
}

void TestNodeVisitor::OutputDirectiveName(const char* name) {
  line_ += "Dir(";
  line_ += name;
  line_ += ")";
}

void TestNodeVisitor::OutputLabel(const assembler::node::Node* node) {
  if (node->label() != nullptr) {
    line_ += " label=";
    line_ += node->label()->value();
  }
}

void TestNodeVisitor::OutputExpression(const assembler::node::Node::TokenList& expr) {
  if (!expr.empty()) {
    line_ += " expr=";
    for (const auto& token : expr) {
      line_ +=  token->value();
    }
  }
}

void TestNodeVisitor::OutputSymbolList(const assembler::node::Node::TokenList& symbols) {
  if (!symbols.empty()) {
    line_ += " symbol_list=";
    bool first = true;
    for (const auto& token : symbols) {
      if (first) {
        first = false;
      } else {
        line_ += ",";
      }
      line_ += token->value();
    }
  }
}

void TestNodeVisitor::OutputInstruction(const assembler::node::Instruction* node) {
  switch (node->format()) {
    case Format::F1:
      line_ += "InsnF1";
      break;
    case Format::F2:
      line_ += "InsnF2";
      break;
    case Format::FS34:
      line_ += "InsnFS34";
      break;
    default:
      FAIL() << "Invalid format id";
      break;
  }
  line_ += "(";
  line_ += node->mnemonic()->value();
  line_ += ")";
  if (node->label() != nullptr) {
    line_ += " label=";
    line_ += node->label()->value();
  }
  line_ += " opcode=";
  line_ += IntToString(node->opcode());
  line_ += " syntax=";
  line_ += IntToString(node->syntax());
}

}  // namespace tests
}  // namespace sicxe
