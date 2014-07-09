#include "assembler/tokenizer.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <string>
#include "assembler/token.h"
#include "common/error_db.h"
#include "common/float_util.h"
#include "common/text_file.h"

using std::string;
using std::unique_ptr;

namespace sicxe {
namespace assembler {

const char* Tokenizer::kOperatorCharacters = "#@,[]=+-*/";

Tokenizer::Tokenizer()
    : tokens_(nullptr), error_db_(nullptr), line_nr_(0), line_(nullptr),
      line_size_(0), state_(INVALID), token_start_(0), position_(0) {}

Tokenizer::~Tokenizer() {}

namespace {

bool* CreateMaskStop() {
  bool* mask = new bool[1 << 8];
  memset(mask, 0, 1 << 8);
  size_t operators_count = strlen(Tokenizer::kOperatorCharacters);
  for (size_t i = 0 ; i < operators_count; i++) {
    mask[static_cast<unsigned char>(Tokenizer::kOperatorCharacters[i])] = true;
  }
  mask[static_cast<unsigned char>(' ')] = true;
  mask[static_cast<unsigned char>('\t')] = true;
  mask[static_cast<unsigned char>('.')] = true;
  return mask;
}

bool* CreateMaskValidName() {
  bool* mask = new bool[1 << 8];
  memset(mask, 0, 1 << 8);
  for (int i = 'a'; i <= 'z'; i++) {
    mask[i] = true;
  }
  for (int i = 'A'; i <= 'Z'; i++) {
    mask[i] = true;
  }
  for (int i = '0'; i <= '9'; i++) {
    mask[i] = true;
  }
  mask[static_cast<unsigned char>('_')] = true;
  return mask;
}

bool* CreateMaskValidHex() {
  bool* mask = new bool[1 << 8];
  memset(mask, 0, 1 << 8);
  for (int i = '0'; i <= '9'; i++) {
    mask[i] = true;
  }
  for (int i = 'a'; i <= 'f'; i++) {
    mask[i] = true;
  }
  for (int i = 'A'; i <= 'F'; i++) {
    mask[i] = true;
  }
  return mask;
}

bool* CreateMaskValidFloat() {
  bool* mask = new bool[1 << 8];
  memset(mask, 0, 1 << 8);
  // allow alpha characters for inputs like "inf"
  for (int i = 'a'; i <= 'z'; i++) {
    mask[i] = true;
  }
  for (int i = 'A'; i <= 'Z'; i++) {
    mask[i] = true;
  }
  for (int i = '0'; i <= '9'; i++) {
    mask[i] = true;
  }
  mask[static_cast<unsigned char>('+')] = true;
  mask[static_cast<unsigned char>('-')] = true;
  mask[static_cast<unsigned char>('.')] = true;
  return mask;
}

}  // namespace

bool Tokenizer::TokenizeLine(const TextFile& file, int line_number,
                             TokenList* tokens, ErrorDB* error_db) {
  assert(line_number < static_cast<int>(file.lines().size()));
  error_db->SetCurrentFile(&file);
  file_ = &file;
  tokens_ = tokens;
  error_db_ = error_db;
  line_nr_ = line_number;
  line_ = file.lines()[line_number].data();
  line_size_ = file.lines()[line_number].size();
  state_ = NONE;
  token_start_ = 0;
  position_ = 0;
  data_char_value_.clear();

  static const bool* mask_stop = CreateMaskStop();
  static const bool* mask_valid_name = CreateMaskValidName();
  static const bool* mask_valid_hex = CreateMaskValidHex();
  static const bool* mask_valid_float = CreateMaskValidFloat();

  for (position_ = 0; position_ <= line_size_; position_++) {
    char current = line_[position_];
    unsigned char current_u = current;

    bool stop = (position_ == line_size_) ||
                (state_ <= INTEGER_HEX && mask_stop[current_u]);

    if (stop) {
      switch (state_) {
        case DATA_CHAR:
        case DATA_HEX:
        case DATA_FLOAT: {
          ErrorUnmatchedQuote();
          return false;
        }
        case COMMENT: {
          CommitComment();
          break;
        }
        case NAME: {
          CommitName();
          break;
        }
        case INTEGER_DEC:
        case INTEGER_BIN:
        case INTEGER_OCT:
        case INTEGER_HEX: {
          if (!CommitInteger()) {
            return false;
          }
          break;
        }
        default: {
          break;
        }
      }
    }

    if (position_ == line_size_) {
      break;
    }

    switch (state_) {
      case NONE: {
        if (!TokenStart()) {
          return false;
        }
        break;
      }
      case NAME: {
        if (!mask_valid_name[current_u]) {
          ErrorNameInvalidChar();
          return false;
        }
        break;
      }
      case INTEGER_DEC: {
        if (!(current >= '0' && current <= '9')) {
          ErrorIntegerDecInvalidChar();
          return false;
        }
        break;
      }
      case INTEGER_BIN: {
        if (!(current >= '0' && current <= '1')) {
          ErrorIntegerBinInvalidChar();
          return false;
        }
        break;
      }
      case INTEGER_OCT: {
        if (!(current >= '0' && current <= '7')) {
          ErrorIntegerOctInvalidChar();
          return false;
        }
        break;
      }
      case INTEGER_HEX: {
        if (!mask_valid_hex[current_u]) {
          ErrorIntegerHexInvalidChar();
          return false;
        }
        break;
      }
      case DATA_CHAR: {
        if (current == '\'') {
          if (!CommitDataChar()) {
            return false;
          }
        } else {
          if (!AdvanceDataChar()) {
            return false;
          }
        }
        break;
      }
      case DATA_HEX: {
        if (current == '\'') {
          if (!CommitDataHex()) {
            return false;
          }
        } else {
          if (!mask_valid_hex[current_u]) {
            ErrorDataHexInvalidChar();
            return false;
          }
        }
        break;
      }
      case DATA_FLOAT: {
        if (current == '\'') {
          if (!CommitDataFloat()) {
            return false;
          }
        } else {
          if (!mask_valid_float[current_u]) {
            ErrorDataFloatInvalidChar();
            return false;
          }
        }
        break;
      }
      case COMMENT: {
        if (!(current >= 0x20 && current <= 0x7e)) {
          ErrorCommentInvalidChar();
          return false;
        }
        break;
      }
      default: {
        assert(false);
        break;
      }
    }
  }
  return true;
}

Tokenizer::StateId* Tokenizer::CreateStartLookupTable() {
  StateId* table = new StateId[1 << 8];
  for (size_t i = 0; i < (1 << 8); i++) {
    table[i] = INVALID;
  }
  size_t operators_count = strlen(Tokenizer::kOperatorCharacters);
  for (size_t i = 0 ; i < operators_count; i++) {
    table[static_cast<unsigned char>(Tokenizer::kOperatorCharacters[i])] = OPERATOR;
  }
  table[static_cast<unsigned char>(' ')] = NONE;
  table[static_cast<unsigned char>('\t')] = NONE;
  table[static_cast<unsigned char>('.')] = COMMENT;
  for (int i = '0'; i <= '9'; i++) {
    table[i] = INTEGER_DEC;
  }
  for (int i = 'a'; i <= 'z'; i++) {
    table[i] = NAME;
  }
  for (int i = 'A'; i <= 'Z'; i++) {
    table[i] = NAME;
  }
  table[static_cast<unsigned char>('_')] = NAME;
  return table;
}

bool Tokenizer::TokenStart() {
  static const StateId* lookup_table = CreateStartLookupTable();
  state_ = lookup_table[static_cast<unsigned char>(line_[position_])];
  token_start_ = position_;
  if (state_ == INVALID) {
    error_db_->AddError(ErrorDB::ERROR, "invalid character",
                        TextFile::Position(line_nr_, position_, 1));
    return false;
  }
  if (state_ == OPERATOR) {
    CommitOperator();
    return true;
  }
  if (position_ < line_size_ - 1) {
    if (state_ == INTEGER_DEC && line_[position_] == '0') {
      switch (line_[position_ + 1]) {
        case 'b':
        case 'B':
          position_++;
          state_ = INTEGER_BIN;
          break;
        case 'o':
        case 'O':
          position_++;
          state_ = INTEGER_OCT;
          break;
        case 'x':
        case 'X':
          position_++;
          state_ = INTEGER_HEX;
          break;
        default:
          break;
      }
    } else if (state_ == NAME && line_[position_ + 1] == '\'') {
      switch (line_[position_]) {
        case 'c':
        case 'C':
          position_++;
          state_ = DATA_CHAR;
          break;
        case 'x':
        case 'X':
          position_++;
          state_ = DATA_HEX;
          break;
        case 'f':
        case 'F':
          position_++;
          state_ = DATA_FLOAT;
          break;
        default:
          break;
      }
    }
  }
  return true;
}

void Tokenizer::ErrorUnmatchedQuote() {
  error_db_->AddError(ErrorDB::ERROR, "unmatched single quote",
                      TextFile::Position(line_nr_, token_start_ + 1, 1));
}

void Tokenizer::ErrorNameInvalidChar() {
  error_db_->AddError(ErrorDB::ERROR,
                      "name may only contain alphanumeric characters and '_'",
                      TextFile::Position(line_nr_, position_, 1));
}

void Tokenizer::ErrorIntegerDecInvalidChar() {
  error_db_->AddError(ErrorDB::ERROR,
                      "integer constant may only contain digits",
                      TextFile::Position(line_nr_, position_, 1));
}

void Tokenizer::ErrorIntegerBinInvalidChar() {
  error_db_->AddError(ErrorDB::ERROR,
                      "binary integer constant may only contain digits '0' and '1'",
                      TextFile::Position(line_nr_, position_, 1));
}

void Tokenizer::ErrorIntegerOctInvalidChar() {
  error_db_->AddError(ErrorDB::ERROR,
                      "octal integer constant may only contain digits '0' through '7'",
                      TextFile::Position(line_nr_, position_, 1));
}

void Tokenizer::ErrorIntegerHexInvalidChar() {
  error_db_->AddError(ErrorDB::ERROR, "hexadecimal integer constant may only contain "
                      "digits and letters 'a' through 'f'",
                      TextFile::Position(line_nr_, position_, 1));
}

void Tokenizer::ErrorDataHexInvalidChar() {
  error_db_->AddError(ErrorDB::ERROR, "invalid character in data constant",
                      TextFile::Position(line_nr_, position_, 1));
}

void Tokenizer::ErrorDataFloatInvalidChar() {
  error_db_->AddError(ErrorDB::ERROR, "invalid character in data constant",
                      TextFile::Position(line_nr_, position_, 1));
}

void Tokenizer::ErrorCommentInvalidChar() {
  error_db_->AddError(ErrorDB::ERROR, "invalid character in comment",
                      TextFile::Position(line_nr_, position_, 1));
}

void Tokenizer::CommitOperator() {
  unique_ptr<Token> token(
      new Token(Token::OPERATOR, *file_, TextFile::Position(line_nr_, token_start_, 1)));

  Token::OperatorId id = Token::OP_ADD;
  switch (token->value()[0]) {
    case '+':
      id = Token::OP_ADD;
      break;
    case '-':
      id = Token::OP_SUB;
      break;
    case '*':
      id = Token::OP_MUL;
      break;
    case '/':
      id = Token::OP_DIV;
      break;
    case '[':
      id = Token::OP_LBRACKET;
      break;
    case ']':
      id = Token::OP_RBRACKET;
      break;
    case ',':
      id = Token::OP_COMMA;
      break;
    case '=':
      id = Token::OP_ASSIGN;
      break;
    case '#':
      id = Token::OP_HASH;
      break;
    case '@':
      id = Token::OP_AT;
      break;
    default:
      assert(false);
      break;
  }
  token->set_operator_id(id);

  tokens_->emplace_back(std::move(token));
  state_ = NONE;
}

void Tokenizer::CommitName() {
  tokens_->emplace_back(new Token(Token::NAME, *file_,
        TextFile::Position(line_nr_, token_start_, position_ - token_start_)));
  state_ = NONE;
}

bool Tokenizer::CommitInteger() {
  unique_ptr<Token> token(new Token(Token::INTEGER, *file_,
        TextFile::Position(line_nr_, token_start_, position_ - token_start_)));

  int base = 0;
  switch (state_) {
    case INTEGER_DEC:
      base = 10;
      break;
    case INTEGER_BIN:
      base = 2;
      break;
    case INTEGER_OCT:
      base = 8;
      break;
    case INTEGER_HEX:
      base = 16;
      break;
    default:
      assert(false);
      break;
  }

  const char* value_str = nullptr;
  if (state_ == INTEGER_DEC) {
    value_str = token->value().c_str();
  } else {
    value_str = token->value().c_str() + 2;
    if (token->value().size() <= 2) {
      error_db_->AddError(ErrorDB::ERROR, "invalid integer constant", token->pos());
      return false;
    }
  }

  char* end_ptr = nullptr;
  errno = 0;
  uint64 value = static_cast<uint64>(strtoull(value_str, &end_ptr, base));
  if (errno == ERANGE || value > 0xffffffff) {
    error_db_->AddError(ErrorDB::ERROR, "integer constant is too large", token->pos());
    return false;
  } else {
    assert(errno == 0);
  }
  assert(*end_ptr == '\0');
  token->set_integer(value);

  tokens_->emplace_back(std::move(token));
  state_ = NONE;
  return true;
}

bool Tokenizer::CommitDataChar() {
  unique_ptr<Token> token(new Token(Token::DATA_BIN, *file_,
        TextFile::Position(line_nr_, token_start_, position_ - token_start_ + 1)));

  if (token->value().size() <= 3) {
    error_db_->AddError(ErrorDB::ERROR, "data constant cannot be empty", token->pos());
    return false;
  }

  *token->mutable_data() = std::move(data_char_value_);
  data_char_value_ = string();

  tokens_->emplace_back(std::move(token));
  state_ = NONE;
  return true;
}

bool Tokenizer::AdvanceDataChar() {
  char current = line_[position_];
  if (current >= 0x20 && current <= 0x7e) {
    if (current == '\\') {
      if (position_ + 1 >= line_size_) {
        error_db_->AddError(ErrorDB::ERROR, "incomplete escape sequence at end of line",
                            TextFile::Position(line_nr_, position_, 1));
        return false;
      }
      char next = line_[position_ + 1];
      switch (next) {
        case '\\':
          data_char_value_.push_back('\\');
          break;
        case 't':
          data_char_value_.push_back('\t');
          break;
        case '\'':
          data_char_value_.push_back('\'');
          break;
        case 'n':
          data_char_value_.push_back('\n');
          break;
        case '0':
          data_char_value_.push_back('\0');
          break;
        default:
          error_db_->AddError(ErrorDB::ERROR, "invalid escape sequence",
                              TextFile::Position(line_nr_, position_, 2));
          return false;
      }
      position_++;
    } else {
      data_char_value_.push_back(current);
    }
    return true;
  } else {
    error_db_->AddError(ErrorDB::ERROR, "invalid character in data constant",
                        TextFile::Position(line_nr_, position_, 1));
    return false;
  }
}

namespace {

char HexDigitValue(char digit) {
  if (digit >= '0' && digit <= '9') {
    return digit - '0';
  } else if (digit >= 'a' && digit <= 'f') {
    return digit - 'a' + 10;
  } else if (digit >= 'A' && digit <= 'F') {
    return digit - 'A' + 10;
  }
  return 0;
}

}  // namespace

bool Tokenizer::CommitDataHex() {
  unique_ptr<Token> token(new Token(Token::DATA_BIN, *file_,
        TextFile::Position(line_nr_, token_start_, position_ - token_start_ + 1)));

  const string& value = token->value();
  if (value.size() <= 3) {
    error_db_->AddError(ErrorDB::ERROR, "data constant cannot be empty", token->pos());
    return false;
  }

  size_t raw_size = value.size() - 3;
  size_t size = (raw_size + 1) / 2;

  string data(size, 0x00);
  size_t i = 0;
  if (raw_size % 2 == 1) {
    data[0] = HexDigitValue(value[2 + 0]);
    i = 1;
  }
  for (size_t j = i; j < size; i += 2, j++) {
    char c = HexDigitValue(value[2 + i]);
    c = (c << 4) | HexDigitValue(value[2 + i + 1]);
    data[j] = c;
  }
  *token->mutable_data() = std::move(data);

  tokens_->emplace_back(std::move(token));
  state_ = NONE;
  return true;
}

bool Tokenizer::CommitDataFloat() {
  unique_ptr<Token> token(new Token(Token::DATA_FLOAT, *file_,
        TextFile::Position(line_nr_, token_start_, position_ - token_start_ + 1)));

  const string& value = token->value();
  if (value.size() <= 3) {
    error_db_->AddError(ErrorDB::ERROR, "data constant cannot be empty", token->pos());
    return false;
  }

  const char* str_ptr = value.c_str();
  char* end_ptr = nullptr;
  double value_double = strtod(str_ptr + 2, &end_ptr);
  if (end_ptr != (str_ptr + (value.size() - 1))) {
    error_db_->AddError(ErrorDB::ERROR, "invalid float data constant", token->pos());
    return false;
  }

  string data(6, 0x00);
  FloatUtil::EncodeFloatData(value_double, reinterpret_cast<uint8*>(&data[0]));
  *token->mutable_data() = std::move(data);

  tokens_->emplace_back(std::move(token));
  state_ = NONE;
  return true;
}

void Tokenizer::CommitComment() {
  tokens_->emplace_back(new Token(Token::COMMENT, *file_,
        TextFile::Position(line_nr_, token_start_, position_ - token_start_)));
  state_ = NONE;
}

}  // namespace assembler
}  // namespace sicxe
