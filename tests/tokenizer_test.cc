#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "assembler/token.h"
#include "assembler/tokenizer.h"
#include "common/error_db.h"
#include "test_util.h"

using sicxe::assembler::Token;
using sicxe::assembler::Tokenizer;
using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {
namespace tests {

class TokenizerTest : public testing::Test {
 protected:
  void InitTextFile(const string& line, TextFile* file) {
    file->set_file_name("test.asm");
    file->mutable_lines()->clear();
    file->mutable_lines()->push_back(line);
  }

  void TestSplit(const string& input, const string& expected) {
    TextFile file;
    InitTextFile(input, &file);

    Tokenizer::TokenList tokens;
    ErrorDB error_db;
    bool success = tokenizer_.TokenizeLine(file, 0, &tokens, &error_db);
    EXPECT_TRUE(success);
    if (success) {
      string actual;
      bool first = true;
      for (const auto& token : tokens) {
        if (first) {
          first = false;
        } else {
          actual += "|";
        }
        actual += token->value();
      }
      EXPECT_EQ(expected, actual);
    }
  }

  void TestError(const string& input, const string& expected_error) {
    TextFile file;
    InitTextFile(input, &file);

    Tokenizer::TokenList tokens;
    ErrorDB error_db;
    bool success = tokenizer_.TokenizeLine(file, 0, &tokens, &error_db);
    EXPECT_FALSE(success);
    if (!success) {
      EXPECT_EQ(expected_error, TestUtil::ErrorDBToStringSimple(error_db));
    }
  }

  Tokenizer tokenizer_;
};

TEST_F(TokenizerTest, Splitting) {
  TestSplit("", "");
  TestSplit("test", "test");
  TestSplit("         \t \t     \t ", "");
  TestSplit("123", "123");
  TestSplit("   \t \t    .    ", ".    ");
  TestSplit("test     ADDR    r1, r2     . sample comment!",
            "test|ADDR|r1|,|r2|. sample comment!");
  TestSplit("  _func     LDA     #var    ",
            "_func|LDA|#|var");
  TestSplit("test     LDX     @[test + 3 * 10]",
            "test|LDX|@|[|test|+|3|*|10|]");
  TestSplit("test     LDF     =F'123.0e10'",
            "test|LDF|=|F'123.0e10'");
  TestSplit("   123+0xabcdef  0b101010-0o76543321+1 \t\t 123.+-*=[]@#",
            "123|+|0xabcdef|0b101010|-|0o76543321|+|1|123|.+-*=[]@#");
  TestSplit("test+-1234*[123]C'ha\\'ha'x'1234'123+f'0.0'.f'0.0'test",
            "test|+|-|1234|*|[|123|]|C'ha\\\'ha'|x'1234'|123|+|f'0.0'|.f'0.0'test");
  TestSplit("  c'123   \\'\\\\\\' 123 '     \t\t\t    ",
            "c'123   \\'\\\\\\' 123 '");
}

TEST_F(TokenizerTest, Errors) {
  // invalid character
  TestError("$", "E[1:1:1];");
  TestError("test 1234; . sample", "E[1:10:1];");
  TestError("x'1234'!1234", "E[1:8:1];");
  TestError("   \n  ", "E[1:4:1];");
  TestError(" \xa0", "E[1:2:1];");
  TestError("  ;_blah", "E[1:3:1];");
  // unmatched quote
  TestError("test     c'   \\'   ", "E[1:11:1];");
  TestError("X'1234", "E[1:2:1];");
  TestError("1234+F'1234.000", "E[1:7:1];");
  // invalid character (name)
  TestError("_FUNC1234_abc$foo+123",  "E[1:14:1];");
  TestError("_func\xeftest+123",  "E[1:6:1];");
  // invalid character (int)
  TestError("1234x5678", "E[1:5:1];");
  TestError("1234\xef+123", "E[1:5:1];");
  // invalid character (int bin)
  TestError("0b10102010101", "E[1:7:1];");
  TestError("0b1010A010101", "E[1:7:1];");
  TestError("0b1010\xefo010101", "E[1:7:1];");
  // invalid character (int oct)
  TestError("0o123456789", "E[1:10:1];");
  TestError("0o123abc", "E[1:6:1];");
  TestError("0o1230\xab", "E[1:7:1];");
  // invalid character (int hex)
  TestError("0x0123456789aBcDefGhI", "E[1:19:1];");
  TestError("0x_", "E[1:3:1];");
  TestError("0x123\xbc", "E[1:6:1];");
  TestError("0x123\x01", "E[1:6:1];");
  // invalid character (data hex)
  TestError("x'123456789abcDEFghijk'", "E[1:18:1];");
  TestError("x'1\xef'", "E[1:4:1];");
  // invalid character (data float)
  TestError("f'*0.0'", "E[1:3:1];");
  TestError("f'$'", "E[1:3:1];");
  TestError("f'\xef'", "E[1:3:1];");
  // invalid character (comment)
  TestError("123 . 1234567890+-!@#$%^&*()[]{}:;=,./~ \x01 ", "E[1:41:1];");
  TestError(".qwertyuiopasdfghjklZXCVBNMM\xab", "E[1:29:1];");
  TestError(".\xef", "E[1:2:1];");
  // invalid int
  TestError("123 0x 123", "E[1:5:2];");
  TestError("test 0O 0b101", "E[1:6:2];");
  TestError("foo 0b 123", "E[1:5:2];");
  // int out of range
  TestError("12345 0x100000000 1234", "E[1:7:11];");
  TestError("5000000000", "E[1:1:10];");
  TestError("0b101010101010101010101010101010101010101010101010", "E[1:1:50];");
  // empty data char
  TestError("test c'' 1234 . test", "E[1:6:3];");
  // data char incomplete escape sequence
  TestError("c'1234\\", "E[1:7:1];");
  // data char invalid escape sequence
  TestError("c'\\\\123\\t\\'\\n\\0\\a'", "E[1:16:2];");
  TestError("c'\\\xef'", "E[1:3:2];");
  // invalid character (data char)
  TestError("c'Test1234 []()-=,./;|!@#$%^&* \\' \n   '", "E[1:35:1];");
  TestError("c'abc\xab'", "E[1:6:1];");
  // empty data hex
  TestError("123 test x'' . test", "E[1:10:3];");
  // empty data float
  TestError("f'0.0' f'' 1234", "E[1:8:3];");
  // invalid dafa float
  TestError("f'0.0' F'-123.0123e-123' f'test' f'0.0'", "E[1:26:7];");
}

TEST_F(TokenizerTest, OperatorId) {
  TextFile file;
  InitTextFile(" + -*  / [], =#@", &file);

  ErrorDB error_db;
  Tokenizer::TokenList token_list;
  ASSERT_TRUE(tokenizer_.TokenizeLine(file, 0, &token_list, &error_db));
  vector<unique_ptr<Token> > tokens(
      std::make_move_iterator(token_list.begin()),
      std::make_move_iterator(token_list.end()));
  EXPECT_EQ(Token::OP_ADD, tokens[0]->operator_id());
  EXPECT_EQ(Token::OP_SUB, tokens[1]->operator_id());
  EXPECT_EQ(Token::OP_MUL, tokens[2]->operator_id());
  EXPECT_EQ(Token::OP_DIV, tokens[3]->operator_id());
  EXPECT_EQ(Token::OP_LBRACKET, tokens[4]->operator_id());
  EXPECT_EQ(Token::OP_RBRACKET, tokens[5]->operator_id());
  EXPECT_EQ(Token::OP_COMMA, tokens[6]->operator_id());
  EXPECT_EQ(Token::OP_ASSIGN, tokens[7]->operator_id());
  EXPECT_EQ(Token::OP_HASH, tokens[8]->operator_id());
  EXPECT_EQ(Token::OP_AT, tokens[9]->operator_id());
}

TEST_F(TokenizerTest, ConvertInt) {
  TextFile file;
  InitTextFile("  1234 0x1234 0o1234 0b01011101   .test", &file);

  ErrorDB error_db;
  Tokenizer::TokenList token_list;
  ASSERT_TRUE(tokenizer_.TokenizeLine(file, 0, &token_list, &error_db));
  vector<unique_ptr<Token> > tokens(
      std::make_move_iterator(token_list.begin()),
      std::make_move_iterator(token_list.end()));
  EXPECT_EQ(1234u, tokens[0]->integer());
  EXPECT_EQ(0x1234u, tokens[1]->integer());
  EXPECT_EQ(01234u, tokens[2]->integer());
  EXPECT_EQ(93u, tokens[3]->integer());
}

TEST_F(TokenizerTest, ConvertData) {
  TextFile file;
  InitTextFile("C'abc\\t \\\\123\\'\\n' X'f12' x'abcd' f'123.0123456e-12'", &file);

  ErrorDB error_db;
  Tokenizer::TokenList token_list;
  ASSERT_TRUE(tokenizer_.TokenizeLine(file, 0, &token_list, &error_db));
  vector<unique_ptr<Token> > tokens(
      std::make_move_iterator(token_list.begin()),
      std::make_move_iterator(token_list.end()));
  EXPECT_EQ("abc\t \\123'\n", tokens[0]->data());
  EXPECT_EQ("\x0f\x12", tokens[1]->data());
  EXPECT_EQ("\xab\xcd", tokens[2]->data());
  EXPECT_EQ("\x3d\xe0\xe8\x1c\xb5\x26", tokens[3]->data());
}

}  // namespace tests
}  // namespace sicxe
