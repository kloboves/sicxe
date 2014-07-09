#ifndef ASSEMBLER_TOKENIZER_H
#define ASSEMBLER_TOKENIZER_H

#include <list>
#include <memory>
#include <string>
#include "common/macros.h"

namespace sicxe {

class ErrorDB;
class TextFile;

namespace assembler {

class Token;

class Tokenizer {
 public:
  DISALLOW_COPY_AND_MOVE(Tokenizer);

  typedef std::list<std::unique_ptr<Token> > TokenList;

  static const char* kOperatorCharacters;

  Tokenizer();
  ~Tokenizer();

  bool TokenizeLine(const TextFile& file, int line_number, TokenList* tokens,
                    ErrorDB* error_db);

 private:
  enum StateId {
    NONE = 0, NAME = 1,
    INTEGER_DEC = 2, INTEGER_BIN = 3, INTEGER_OCT = 4, INTEGER_HEX = 5,
    DATA_CHAR = 6, DATA_HEX = 7, DATA_FLOAT = 8, COMMENT = 9,
    // pseudo states:
    OPERATOR = 10, INVALID = 11
  };

  StateId* CreateStartLookupTable();
  bool TokenStart();

  void ErrorUnmatchedQuote();
  void ErrorNameInvalidChar();
  void ErrorIntegerDecInvalidChar();
  void ErrorIntegerBinInvalidChar();
  void ErrorIntegerOctInvalidChar();
  void ErrorIntegerHexInvalidChar();
  void ErrorDataHexInvalidChar();
  void ErrorDataFloatInvalidChar();
  void ErrorCommentInvalidChar();

  void CommitOperator();
  void CommitName();
  bool CommitInteger();
  bool CommitDataChar();
  bool AdvanceDataChar();
  bool CommitDataHex();
  bool CommitDataFloat();
  void CommitComment();

  const TextFile* file_;
  TokenList* tokens_;
  ErrorDB* error_db_;
  int line_nr_;
  const char* line_;
  size_t line_size_;
  StateId state_;
  size_t token_start_;  // start of current token
  size_t position_;  // current position in line
  std::string data_char_value_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_TOKENIZER_H
