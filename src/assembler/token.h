#ifndef ASSEMBLER_TOKEN_H
#define ASSEMBLER_TOKEN_H

#include <string>
#include "common/macros.h"
#include "common/text_file.h"
#include "common/types.h"

namespace sicxe {
namespace assembler {

class Token {
 public:
  DISALLOW_COPY_AND_MOVE(Token);

  enum TypeId {
    NAME = 0,
    OPERATOR,
    COMMENT,
    INTEGER,
    DATA_BIN,
    DATA_FLOAT
  };

  enum OperatorId {
    OP_ADD = 0,
    OP_SUB = 1,
    OP_MUL = 2,
    OP_DIV = 3,
    OP_LBRACKET = 4,
    OP_RBRACKET = 5,
    OP_COMMA = 6,
    OP_ASSIGN = 7,
    OP_HASH = 8,
    OP_AT = 9
  };

  Token(TypeId the_type, const TextFile& text_file, const TextFile::Position& the_pos);
  ~Token();

  TypeId type() const;
  const TextFile::Position& pos() const;
  const std::string& value() const;
  OperatorId operator_id() const;
  void set_operator_id(OperatorId id);
  uint32 integer() const;
  void set_integer(uint32 the_integer);
  const std::string& data() const;
  void set_data(const std::string& the_data);
  std::string* mutable_data();

 private:
  TypeId type_;
  TextFile::Position pos_;
  std::string value_;
  OperatorId operator_id_;
  uint32 integer_;
  std::string data_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_TOKEN_H
