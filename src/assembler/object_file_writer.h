#ifndef ASSEMBLER_OBJECT_MODULE_WRITER_H
#define ASSEMBLER_OBJECT_MODULE_WRITER_H

#include <stdio.h>
#include <memory>
#include "assembler/code_generator.h"
#include "common/macros.h"
#include "common/object_file.h"

namespace sicxe {

namespace assembler {

class Code;

class ObjectFileWriter : public CodeOutputWriter {
 public:
  DISALLOW_COPY_AND_MOVE(ObjectFileWriter);

  explicit ObjectFileWriter(ObjectFile* object_file);
  ~ObjectFileWriter();

 private:
  virtual void Begin(const Code& code);
  virtual void WriteNode(const node::Node& node, uint32 address);
  virtual void WriteNode(const node::Node& node, uint32 address,
                         const std::string& data, bool can_split);
  virtual void WriteRelocationRecord(const RelocationRecord& record);
  virtual void End();

  void CommitTextSection();

  const Code* code_;
  ObjectFile* object_file_;
  std::unique_ptr<uint8[]> text_buffer_;
  uint32 text_address_;
  int text_position_;
  uint32 text_next_address_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_OBJECT_MODULE_WRITER_H
