#include <gtest/gtest.h>

#include <vector>
#include "assembler/code.h"
#include "assembler/parser.h"
#include "common/error_db.h"
#include "common/instruction_db.h"
#include "common/text_file.h"
#include "test_node_visitor.h"
#include "test_util.h"

using std::vector;
using namespace sicxe::assembler;

namespace sicxe {
namespace tests {

struct ParserTestParams {
  ParserTestParams(int in, bool cs, bool ab, bool s)
    : input_number(in), case_sensitive(cs), allow_brackets(ab), success(s) {}

  int input_number;
  bool case_sensitive;
  bool allow_brackets;
  bool success;
};

std::ostream& operator<<(std::ostream& stream, const ParserTestParams& params) {
  stream << "[number=" << params.input_number << "; case_sensitive="
         << params.case_sensitive << "; allow_brackets=" << params.allow_brackets
         << "; success=" << params.success << "]";
  return stream;
}

class ParserTest : public testing::TestWithParam<ParserTestParams> {
 protected:
  ParserTest() {
    config_.instruction_db = InstructionDB::Default();
  }

  Parser::Config config_;
  TextFile input_file_;
  TextFile output_file_;
  TextFile expected_file_;
  Code code_;
  ErrorDB error_db_;
};

TEST_P(ParserTest, Parse) {
  char buffer[200];
  const ParserTestParams& params = GetParam();
  config_.case_sensitive = params.case_sensitive;
  config_.allow_brackets = params.allow_brackets;
  if (params.success) {
    snprintf(buffer, sizeof(buffer), "parser-in-%d-good.asm", params.input_number);
  } else {
    snprintf(buffer, sizeof(buffer), "parser-in-%d-bad.asm", params.input_number);
  }
  ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &input_file_));
  Parser p(&config_);
  if (params.success) {
    ASSERT_TRUE(p.ParseFile(input_file_, &code_, &error_db_));
    snprintf(buffer, sizeof(buffer), "parser-out-%d-good.txt", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &expected_file_));
    TestNodeVisitor::DumpCode(code_, &output_file_);
  } else {
    ASSERT_FALSE(p.ParseFile(input_file_, &code_, &error_db_));
    snprintf(buffer, sizeof(buffer), "parser-err-%d-bad.txt", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &expected_file_));
    TestUtil::ErrorDBToTextFile(error_db_, &output_file_);
  }
  ASSERT_PRED_FORMAT2(TestUtil::FilesEqual, expected_file_, output_file_);
}

namespace {

vector<ParserTestParams> GetGoodInputs() {
  vector<ParserTestParams> inputs{
    ParserTestParams(1, true, true, true),
    ParserTestParams(2, false, false, true),
  };
  return inputs;
}

vector<ParserTestParams> GetBadInputs() {
  vector<ParserTestParams> inputs{
    ParserTestParams(1, true, true, false),
    ParserTestParams(2, true, true, false),
    ParserTestParams(3, true, true, false),
    ParserTestParams(4, true, true, false),
    ParserTestParams(5, true, false, false),
    ParserTestParams(6, true, true, false),
    ParserTestParams(7, true, true, false),
    ParserTestParams(8, true, true, false),
  };
  return inputs;
}

}  // namespace

INSTANTIATE_TEST_CASE_P(Good, ParserTest, ::testing::ValuesIn(GetGoodInputs()));
INSTANTIATE_TEST_CASE_P(Bad, ParserTest, ::testing::ValuesIn(GetBadInputs()));

}  // namespace tests
}  // namespace sicxe
