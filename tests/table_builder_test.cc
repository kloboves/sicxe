#include <gtest/gtest.h>

#include <vector>
#include "assembler/code.h"
#include "assembler/parser.h"
#include "assembler/table_builder.h"
#include "common/error_db.h"
#include "common/instruction_db.h"
#include "test_node_visitor.h"
#include "test_util.h"

using std::vector;
using namespace sicxe::assembler;

namespace sicxe {
namespace tests {

struct TableBuilderTestParams {
  TableBuilderTestParams(int in, bool s)
    : input_number(in), success(s) {}

  int input_number;
  bool success;
};

std::ostream& operator<<(std::ostream& stream, const TableBuilderTestParams& params) {
  stream << "[number=" << params.input_number << "; success=" << params.success << "]";
  return stream;
}

class TableBuilderTest : public testing::TestWithParam<TableBuilderTestParams> {
 protected:
  TableBuilderTest() {
    config_.instruction_db = InstructionDB::Default();
  }

  Parser::Config config_;
  TextFile input_file_;
  TextFile output_file_;
  TextFile expected_file_;
  Code code_;
  ErrorDB error_db_;
};

TEST_P(TableBuilderTest, BuildTables) {
  char buffer[200];
  const TableBuilderTestParams& params = GetParam();
  config_.case_sensitive = true;
  config_.allow_brackets = true;
  if (params.success) {
    snprintf(buffer, sizeof(buffer), "tbuilder-in-%d-good.asm", params.input_number);
  } else {
    snprintf(buffer, sizeof(buffer), "tbuilder-in-%d-bad.asm", params.input_number);
  }
  ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &input_file_));
  Parser p(&config_);
  ASSERT_TRUE(p.ParseFile(input_file_, &code_, &error_db_));
  TableBuilder table_builder;
  if (params.success) {
    ASSERT_TRUE(table_builder.BuildTables(&code_, &error_db_));
    snprintf(buffer, sizeof(buffer), "tbuilder-out-%d-good.txt", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &expected_file_));
    TestNodeVisitor::DumpCodeAndTables(code_, &output_file_);
  } else {
    ASSERT_FALSE(table_builder.BuildTables(&code_, &error_db_));
    snprintf(buffer, sizeof(buffer), "tbuilder-err-%d-bad.txt", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &expected_file_));
    TestUtil::ErrorDBToTextFile(error_db_, &output_file_);
  }
  ASSERT_PRED_FORMAT2(TestUtil::FilesEqual, expected_file_, output_file_);
}

namespace {

vector<TableBuilderTestParams> GetInputs(int from, int to, bool success) {
  vector<TableBuilderTestParams> inputs;
  for (int i = from; i <= to; i++) {
    inputs.push_back(TableBuilderTestParams(i, success));
  }
  return inputs;
}

}  // namespace

INSTANTIATE_TEST_CASE_P(Good, TableBuilderTest,
                        ::testing::ValuesIn(GetInputs(1, 3, true)));
INSTANTIATE_TEST_CASE_P(Bad, TableBuilderTest,
                        ::testing::ValuesIn(GetInputs(1, 29, false)));

}  // namespace tests
}  // namespace sicxe
