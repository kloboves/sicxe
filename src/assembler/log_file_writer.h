#ifndef ASSEMBLER_LOG_FILE_WRITER_H
#define ASSEMBLER_LOG_FILE_WRITER_H

#include <memory>
#include <string>
#include "assembler/code_generator.h"
#include "assembler/node_visitor.h"
#include "common/macros.h"

namespace sicxe {

class InstructionDB;
class TextFile;

namespace assembler {

class Code;

class LogFileWriter : public CodeOutputWriter, public ConstNodeVisitor {
 public:
  DISALLOW_COPY_AND_MOVE(LogFileWriter);

  LogFileWriter(const InstructionDB* instruction_db, TextFile* text_file);
  ~LogFileWriter();

 private:
  virtual void Begin(const Code& code);
  virtual void WriteNode(const node::Node& node, uint32 address);
  virtual void WriteNode(const node::Node& node, uint32 address,
                         const std::string& data, bool can_split);
  virtual void WriteRelocationRecord(const RelocationRecord& record);
  virtual void End();

  virtual void VisitNode(const node::Empty* node);
  virtual void VisitNode(const node::InstructionF1* node);
  virtual void VisitNode(const node::InstructionF2* node);
  virtual void VisitNode(const node::InstructionFS34* node);
  virtual void VisitNode(const node::DirectiveStart* node);
  virtual void VisitNode(const node::DirectiveEnd* node);
  virtual void VisitNode(const node::DirectiveOrg* node);
  virtual void VisitNode(const node::DirectiveEqu* node);
  virtual void VisitNode(const node::DirectiveUse* node);
  virtual void VisitNode(const node::DirectiveLtorg* node);
  virtual void VisitNode(const node::DirectiveBase* node);
  virtual void VisitNode(const node::DirectiveNobase* node);
  virtual void VisitNode(const node::DirectiveExtdef* node);
  virtual void VisitNode(const node::DirectiveExtref* node);
  virtual void VisitNode(const node::DirectiveMemInit* node);
  virtual void VisitNode(const node::DirectiveMemReserve* node);
  virtual void VisitNode(const node::DirectiveInternalLiteral* node);

  void AddSpaces(size_t columns);
  void LineWriteAddress();
  void LineWriteData();
  void LineWriteLabel(const std::string& label);
  void LineWriteInstruction(const node::Instruction* node, bool extended);
  void LineWriteDirective(const node::Directive* node);
  void LineWriteExpression(const node::Node::TokenList& expression);
  void LineWriteSymbolList(const node::Node::TokenList& symbols);
  void LineWriteDataLiteral(const std::string& data);
  void LineWriteComment(const node::Node* node);

  const InstructionDB* instruction_db_;
  TextFile* text_file_;
  const Code* code_;
  std::string line_;
  std::unique_ptr<char[]> buffer_;
  bool has_data_;
  const std::string* data_;
  uint32 address_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_LOG_FILE_WRITER_H
