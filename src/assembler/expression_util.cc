#include "assembler/expression_util.h"

#include <map>
#include "assembler/symbol_table.h"
#include "assembler/token.h"
#include "common/error_db.h"

using std::map;
using std::string;

namespace sicxe {
namespace assembler {

ExpressionUtil::ExternalSymbol::ExternalSymbol(SignId the_sign,
                                               const string& the_name)
    : sign(the_sign), name(the_name) {}

namespace {

// Checks that all symbols in the expression are defined and their value is known.
// If there are any external symbols in the expression sets |has_external| to true.
bool ValidateExpressionSymbols(const ExpressionUtil::TokenList& expression,
                               const SymbolTable& symbol_table,
                               bool allow_external, bool* has_external,
                               ErrorDB* error_db) {
  bool success = true;
  for (const auto& token : expression) {
    if (token->type() == Token::NAME) {
      const SymbolTable::Entry* entry = symbol_table.Find(token->value());
      if (entry == nullptr || entry->type == SymbolTable::UNKNOWN ||
          (entry->type == SymbolTable::INTERNAL && !entry->defined)) {
        error_db->AddError(ErrorDB::ERROR, "undefined symbol", token->pos());
        success = false;
        continue;
      }
      if (entry->type == SymbolTable::EXTERNAL) {
        *has_external = true;
        if (!allow_external) {
          error_db->AddError(ErrorDB::ERROR,
                             "imported external symbols not allowed in this expression",
                             token->pos());
          success = false;
          continue;
        }
      } else if (entry->type == SymbolTable::INTERNAL) {
        if (!entry->resolved) {
          error_db->AddError(ErrorDB::ERROR,
                             "symbols from blocks not allowed in this expression",
                             token->pos());
          success = false;
          continue;
        }
      }
    }
  }
  return success;
}

bool PeekIsEndOfTerm(const ExpressionUtil::TokenList& expression,
                     const ExpressionUtil::TokenList::const_iterator it) {
  ExpressionUtil::TokenList::const_iterator next_it = it;
  ++next_it;
  if (next_it == expression.end()) {
    return true;
  } else {
    const Token& token = *next_it->get();
    assert(token.type() == Token::OPERATOR);
    return token.operator_id() == Token::OP_ADD ||
           token.operator_id() == Token::OP_SUB;
  }
}

bool CheckOverflow(int64 value, const ExpressionUtil::TokenList& expression,
                   ErrorDB* error_db) {
  if (value < -2147483648LL || value > 2147483647LL) {
    error_db->AddError(ErrorDB::ERROR, "overflow while evaluating expression",
                       expression.front()->pos(), expression.back()->pos());
    return false;
  }
  return true;
}

bool GetTermTokenValue(const Token& token, const SymbolTable& symbol_table,
                         int64* value, ErrorDB* error_db) {
  if (token.type() == Token::INTEGER) {
    *value = token.integer();
  } else if (token.type() == Token::NAME) {
    const SymbolTable::Entry* entry = symbol_table.Find(token.value());
    assert(entry != nullptr);
    if (entry->type == SymbolTable::EXTERNAL) {
      error_db->AddError(ErrorDB::ERROR,
                         "external imported symbols must not be operands to '*' and '/'",
                         token.pos());
      return false;
    }
    assert(entry->type == SymbolTable::INTERNAL);
    if (entry->relative) {
      error_db->AddError(ErrorDB::ERROR,
                         "relative symbols must not be operands to '*' and '/'",
                         token.pos());
      return false;
    }
    assert(entry->resolved);
    *value = entry->address;
  } else {
    assert(false);
  }
  return true;
}

bool SolveTerm(const ExpressionUtil::TokenList& expression,
                 const SymbolTable& symbol_table,
                 ExpressionUtil::TokenList::const_iterator* it,
                 int64* value_ptr, ErrorDB* error_db) {
  int64 value = 0;
  if (!GetTermTokenValue(*(*it)->get(), symbol_table, &value, error_db) ||
      !CheckOverflow(value, expression, error_db)) {
    return false;
  }
  while (true) {
    if (PeekIsEndOfTerm(expression, *it)) {
      break;
    }
    ++(*it);
    const Token& operator_token = *(*it)->get();
    assert(operator_token.type() == Token::OPERATOR);
    ++(*it);
    const Token& operand_token = *(*it)->get();
    int64 operand_value = 0;
    if (!GetTermTokenValue(operand_token, symbol_table, &operand_value, error_db) ||
        !CheckOverflow(operand_value, expression, error_db)) {
        return false;
    }
    if (operator_token.operator_id() == Token::OP_MUL) {
      value *= operand_value;
    } else if (operator_token.operator_id() == Token::OP_DIV) {
      if (operand_value == 0) {
        error_db->AddError(ErrorDB::ERROR, "division by zero", operand_token.pos());
        return false;
      }
      value /= operand_value;
    } else {
      assert(false);
    }
    if (!CheckOverflow(value, expression, error_db)) {
      return false;
    }
  }
  *value_ptr = value;
  return true;
}

bool SolveSingle(const Token& token, const SymbolTable& symbol_table,
                 ExpressionUtil::SignId sign, bool has_external, int64* value,
                 bool* relative, map<string, int>* external_map_ptr, ErrorDB* error_db) {
  if (token.type() == Token::INTEGER) {
    *relative = false;
    *value = token.integer();
  } else if (token.type() == Token::NAME) {
    const SymbolTable::Entry* entry = symbol_table.Find(token.value());
    assert(entry != nullptr);
    if (has_external &&
        (entry->type == SymbolTable::EXTERNAL ||
         (entry->type == SymbolTable::INTERNAL && entry->exported))) {
      *relative = true;
      (*external_map_ptr)[token.value()] += (sign == ExpressionUtil::PLUS) ? 1 : -1;
    } else {
      assert(entry->type == SymbolTable::INTERNAL);
      if (has_external && entry->relative) {
        error_db->AddError(ErrorDB::ERROR,
                           "non-exported relative symbols not allowed in expression that "
                           "contains external imported symbols", token.pos());
        return false;
      }
      assert(entry->resolved);
      *relative = entry->relative;
      *value = entry->address;
    }
  } else {
    assert(false);
  }
  return true;
}

}  // namespace

bool ExpressionUtil::Solve(const TokenList& expression, const SymbolTable& symbol_table,
                           bool allow_external, int32* result, bool* relative,
                           ExternalSymbolVector* external_symbols, ErrorDB* error_db) {
  if (allow_external) {
    assert(external_symbols != nullptr);
  }
  bool has_external = false;
  if (!ValidateExpressionSymbols(expression, symbol_table, allow_external,
                                 &has_external, error_db)) {
    return false;
  }

  int64 value = 0;
  int relative_count = 0;
  SignId sign = PLUS;
  map<string, int> external_map;

  for (auto it = expression.begin(); it != expression.end(); ++it) {
    const Token& token = *it->get();
    if (token.type() == Token::OPERATOR) {
      switch (token.operator_id()) {
        case Token::OP_ADD:
          sign = PLUS;
          break;
        case Token::OP_SUB:
          sign = MINUS;
          break;
        default:
          assert(false);
          break;
      }
    } else if (token.type() == Token::NAME || token.type() == Token::INTEGER) {
      bool term_single = PeekIsEndOfTerm(expression, it);
      bool term_relative = false;
      int64 term_value = 0;
      if (term_single) {
        if (!SolveSingle(token, symbol_table, sign, has_external, &term_value,
                         &term_relative, &external_map, error_db)) {
          return false;
        }
      } else {
        if (!SolveTerm(expression, symbol_table, &it, &term_value, error_db)) {
          return false;
        }
      }
      value += (sign == PLUS) ? term_value : -term_value;
      if (term_relative) {
        relative_count += (sign == PLUS) ? 1 : -1;
      }
      if (!CheckOverflow(value, expression, error_db)) {
        return false;
      }
    } else {
      assert(false);
    }
  }

  if (relative_count == 0) {
    *relative = false;
  } else if (relative_count == 1) {
    *relative = true;
  } else {
    error_db->AddError(ErrorDB::ERROR, "expression is neither relative nor absolute",
                       expression.front()->pos(), expression.back()->pos());
    return false;
  }
  *result = static_cast<int32>(value);
  for (const auto& symbol : external_map) {
    SignId symbol_sign = PLUS;
    int count = 0;
    if (symbol.second > 0) {
      symbol_sign = PLUS;
      count = symbol.second;
    } else if (symbol.second < 0) {
      symbol_sign = MINUS;
      count = -symbol.second;
    }
    for (int i = 0; i < count; i++) {
      external_symbols->push_back(ExternalSymbol(symbol_sign, symbol.first));
    }
  }
  return true;
}

}  // namespace assembler
}  // namespace sicxe
