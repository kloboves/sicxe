#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "common/command_interface.h"
#include "test_util.h"

using std::string;
using std::vector;

namespace sicxe {
namespace tests {

class CommandInterfaceTest : public testing::Test {
 protected:
  void TestTokenize(const string& str, const string& expect_tokens) {
    vector<string> tokens;
    EXPECT_TRUE(CommandInterface::TokenizeInput(str.data(), str.length(), &tokens));
    EXPECT_EQ(expect_tokens, TestUtil::ConcatStrVector(tokens, "|"));
  }

  void TestParse(const string& str, const string& expect_path,
                 const string& expect_args) {
    vector<string> tokens;
    EXPECT_TRUE(CommandInterface::TokenizeInput(str.data(), str.length(), &tokens));
    vector<string> path;
    CommandInterface::ParsedArgumentMap args;
    EXPECT_TRUE(CommandInterface::ParseInput(tokens, &path, &args));
    EXPECT_EQ(expect_path, TestUtil::ConcatStrVector(path, "/"));
    vector<string> args_str;
    for (const auto& a : args) {
      args_str.push_back(a.first + ":" + a.second.value_str);
    }
    EXPECT_EQ(expect_args, TestUtil::ConcatStrVector(args_str, "|"));
  }
};

TEST_F(CommandInterfaceTest, Tokenizer) {
  TestTokenize("some command arg1=test arg2=\"bla bla\"",
               "some|command|arg1|=|test|arg2|=|bla bla");
  TestTokenize("cpu print", "cpu|print");
  TestTokenize("memory print ?", "memory|print|?");
  TestTokenize("\"test test\" \"test\"=\"test1 test2\"",
               "test test|test|=|test1 test2");
}

TEST_F(CommandInterfaceTest, Parser) {
  TestParse("cpu print", "cpu/print", "");
  TestParse("cpu print register=a value=100", "cpu/print",
            "register:a|value:100");
  TestParse("memory print ?", "memory/print/?", "");
  TestParse("test foo=\"a b c d e\" \"bar\"=\"test\"", "test", "bar:test|foo:a b c d e");
}

}  // namespace tests
}  // namespace sicxe
