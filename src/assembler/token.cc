#include "assembler/token.h"

#include <memory>

using std::string;

namespace sicxe {
namespace assembler {

Token::Token(TypeId the_type, const TextFile& text_file,
             const TextFile::Position& the_pos)
    : type_(the_type), pos_(the_pos),
      value_(text_file.lines()[pos_.row].substr(pos_.column, pos_.size)),
      integer_(0) {}

Token::~Token() {}

Token::TypeId Token::type() const {
  return type_;
}

const TextFile::Position& Token::pos() const {
  return pos_;
}

const string& Token::value() const {
  return value_;
}

Token::OperatorId Token::operator_id() const {
  return operator_id_;
}

void Token::set_operator_id(OperatorId id) {
  operator_id_ = id;
}

uint32 Token::integer() const {
  return integer_;
}

void Token::set_integer(uint32 the_integer) {
  integer_ = the_integer;
}

const string& Token::data() const {
  return data_;
}

void Token::set_data(const std::string& the_data) {
  data_ = the_data;
}

string* Token::mutable_data() {
  return &data_;
}

}  // namespace assembler
}  // namespace sicxe
