#ifndef ASSEMBLER_EXPRESSION_UTIL_H
#define ASSEMBLER_EXPRESSION_UTIL_H

#include <list>
#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class ErrorDB;

namespace assembler {

class Token;
class SymbolTable;

class ExpressionUtil {
 public:
  DISALLOW_INSTANTIATE(ExpressionUtil);

  enum SignId {
    PLUS = 0,
    MINUS
  };

  struct ExternalSymbol {
    ExternalSymbol(SignId the_sign, const std::string& the_name);

    SignId sign;
    std::string name;
  };

  typedef std::vector<ExternalSymbol> ExternalSymbolVector;
  typedef std::list<std::unique_ptr<Token> > TokenList;

  // Assumes |error_db|'s current file is set correctly
  static bool Solve(const TokenList& expression, const SymbolTable& symbol_table,
                    bool allow_external, int32* result, bool* relative,
                    ExternalSymbolVector* external_symbols, ErrorDB* error_db);
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_EXPRESSION_UTIL_H
