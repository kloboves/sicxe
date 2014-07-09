#include <gtest/gtest.h>

#include <string>
#include "assembler/expression_util.h"
#include "assembler/symbol_table.h"
#include "assembler/token.h"
#include "assembler/tokenizer.h"
#include "common/error_db.h"
#include "common/text_file.h"
#include "test_util.h"

using std::string;
using namespace sicxe::assembler;

namespace sicxe {
namespace tests {

class ExpressionUtilTest : public testing::Test {
 protected:
  ExpressionUtilTest() : result_(0), relative_(false) {
    // internal symbols:
    CreateSymbolInternal("a", 21, false, false);
    CreateSymbolInternal("b", 32, false, false);
    CreateSymbolInternal("c", 0, false, false);
    CreateSymbolInternal("d", 42, true, false);
    CreateSymbolInternal("e", 14, true, false);
    CreateSymbolInternal("f", 51, true, true);
    CreateSymbolInternal("g", 27, true, true);
    // block symbols
    CreateSymbolInternal("h", 10, true, false, 3);
    CreateSymbolInternal("i", 20, true, true, 4);
    // exported undefined symbols:
    CreateSymbolExported("k");
    CreateSymbolExported("l");
    // imported symbols:
    CreateSymbolImported("m");
    CreateSymbolImported("n");
    // unknown symbols:
    CreateSymbolUnknown("o");
    CreateSymbolUnknown("p");
    CreateSymbolUnknown("r");
  }

  void CreateSymbolInternal(const string& name, uint32 value, bool relative,
                            bool exported) {
    CreateSymbolInternal(name, value, relative, exported, -1);
  }

  void CreateSymbolInternal(const string& name, uint32 value, bool relative,
                            bool exported, int block) {
    SymbolTable::Entry* entry = symbol_table_.FindOrCreateNew(name);
    entry->type = SymbolTable::INTERNAL;
    entry->relative = relative;
    entry->defined = true;
    entry->address = value;
    entry->exported = exported;
    if (block > 0) {
      entry->block_address = value;
      entry->resolved = false;
    } else {
      entry->resolved = true;
    }
  }

  void CreateSymbolExported(const string& name) {
    SymbolTable::Entry* entry = symbol_table_.FindOrCreateNew(name);
    entry->type = SymbolTable::INTERNAL;
    entry->relative = true;
    entry->exported = true;
    entry->defined = false;
    entry->resolved = false;
  }

  void CreateSymbolImported(const string& name) {
    SymbolTable::Entry* entry = symbol_table_.FindOrCreateNew(name);
    entry->type = SymbolTable::EXTERNAL;
    entry->relative = true;
  }

  void CreateSymbolUnknown(const string& name) {
    SymbolTable::Entry* entry = symbol_table_.FindOrCreateNew(name);
    entry->type = SymbolTable::UNKNOWN;
  }

  bool Solve(const string& expression_str, bool allow_external) {
    input_file_.set_file_name("test.txt");
    input_file_.mutable_lines()->push_back(expression_str);
    Tokenizer tokenizer;
    if (!tokenizer.TokenizeLine(input_file_, 0, &expression_, &error_db_)) {
      ADD_FAILURE() << "Can't tokenize expression!";
      return false;
    }
    return ExpressionUtil::Solve(expression_, symbol_table_, allow_external, &result_,
                                 &relative_, &external_symbols_, &error_db_);
  }

  string ExternalSymStr() {
    string rv;
    for (const auto& sym : external_symbols_) {
      switch (sym.sign) {
        case ExpressionUtil::PLUS:
          rv += "+";
          break;
        case ExpressionUtil::MINUS:
          rv += "-";
          break;
      }
      rv += sym.name;
    }
    return rv;
  }

  SymbolTable symbol_table_;

  TextFile input_file_;
  Tokenizer::TokenList expression_;
  ErrorDB error_db_;
  int32 result_;
  bool relative_;
  ExpressionUtil::ExternalSymbolVector external_symbols_;
};

TEST_F(ExpressionUtilTest, Solve1Bad) {
  // contains undefined symbols and symbols from blocks
  ASSERT_FALSE(Solve("12 * 15 + 3 * x + k + o * 2 - m + 2 * n + h + 2 * i", true));
  ASSERT_EQ("E[1:15:1];E[1:19:1];E[1:23:1];E[1:43:1];E[1:51:1];",
            TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve2Bad) {
  // contains undefined symbols and imported symbols
  ASSERT_FALSE(Solve("x + m - n + 3 * 15", false));
  ASSERT_EQ("E[1:1:1];E[1:5:1];E[1:9:1];",
            TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve3Bad) {
  // positive overflow
  ASSERT_FALSE(Solve("1300 * 1300 * 1300", true));
  ASSERT_EQ("E[1:1:18];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve4Bad) {
  // negative overflow
  ASSERT_FALSE(Solve("-750 * 1300 * 1300 - 750 * 1300 * 1300", true));
  ASSERT_EQ("E[1:1:38];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve5Bad) {
  // positive overflow
  ASSERT_FALSE(Solve("750 * 1300 * 1300 + 750 * 1300 * 1300", true));
  ASSERT_EQ("E[1:1:37];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve6Bad) {
  // multiplication of external imported symbols
  ASSERT_FALSE(Solve("10 * 20 + 3 * m + 3", true));
  ASSERT_EQ("E[1:15:1];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve7Bad) {
  // multiplication of relative symbols
  ASSERT_FALSE(Solve("5 * a + a * b + d * 3 + 5", true));
  ASSERT_EQ("E[1:17:1];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve8Bad) {
  // division by zero (c = 0)
  ASSERT_FALSE(Solve("100 + a / c", true));
  ASSERT_EQ("E[1:11:1];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve9Bad) {
  // division by zero (c = 0)
  ASSERT_FALSE(Solve("100 + a / c", true));
  ASSERT_EQ("E[1:11:1];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve10Bad) {
  // mixing imported symbols with non-exported relative symbols
  ASSERT_FALSE(Solve("3 * 4 + a * b - d + e + f - m + 3", true));
  ASSERT_EQ("E[1:17:1];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve11Bad) {
  // expression not absolute or relative
  ASSERT_FALSE(Solve("a + b - c + 3 * 10 + d + f + 10", true));
  ASSERT_EQ("E[1:1:31];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve12Bad) {
  // expression not absolute or relative
  ASSERT_FALSE(Solve("a + b - c + 3 * 10 + f - m - n + 10", true));
  ASSERT_EQ("E[1:1:35];", TestUtil::ErrorDBToStringSimple(error_db_));
}

TEST_F(ExpressionUtilTest, Solve13Good) {
  ASSERT_TRUE(Solve("- 132 + 62 * 123 / 3 - 9 * 41 + 100 / 7 * 18 - 100", true));
  ASSERT_FALSE(relative_);
  ASSERT_EQ(2193, result_);
  ASSERT_TRUE(external_symbols_.empty());
}

TEST_F(ExpressionUtilTest, Solve14Good) {
  ASSERT_TRUE(Solve("1000 + 100 + 10 + 1 - 13 * 4000 / 12", true));
  ASSERT_FALSE(relative_);
  ASSERT_EQ(-3222, result_);
  ASSERT_TRUE(external_symbols_.empty());
}

TEST_F(ExpressionUtilTest, Solve15Good) {
  ASSERT_TRUE(Solve("a * 123 * a - b * 100 * b / 7 + a - 2 * b", true));
  ASSERT_FALSE(relative_);
  ASSERT_EQ(39572, result_);
  ASSERT_TRUE(external_symbols_.empty());
}

TEST_F(ExpressionUtilTest, Solve16Good) {
  ASSERT_TRUE(Solve("12 * 10 + a * 5 + e - f", true));
  ASSERT_FALSE(relative_);
  ASSERT_EQ(188, result_);
  ASSERT_TRUE(external_symbols_.empty());
}

TEST_F(ExpressionUtilTest, Solve17Good) {
  ASSERT_TRUE(Solve("3 + f - e + 4 * 3 + g", true));
  ASSERT_TRUE(relative_);
  ASSERT_EQ(79, result_);
  ASSERT_TRUE(external_symbols_.empty());
}

TEST_F(ExpressionUtilTest, Solve18Good) {
  ASSERT_TRUE(Solve("10 + f + f - f - f - f - f + f + m + m - n + n + n - g", true));
  ASSERT_TRUE(relative_);
  ASSERT_EQ(10, result_);
  ASSERT_EQ("-f-g+m+m+n", ExternalSymStr());
}

TEST_F(ExpressionUtilTest, Solve19Good) {
  ASSERT_TRUE(Solve("4 * a + m + n - m - n + f -f + f -f", true));
  ASSERT_FALSE(relative_);
  ASSERT_EQ(84, result_);
  ASSERT_TRUE(external_symbols_.empty());
}

}  // namespace tests
}  // namespace sicxe
