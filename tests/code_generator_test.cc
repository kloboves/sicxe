#include <gtest/gtest.h>

#include <string>
#include <vector>
#include "assembler/code_generator.h"
#include "assembler/log_file_writer.h"
#include "assembler/parser.h"
#include "assembler/table_builder.h"
#include "common/error_db.h"
#include "common/instruction_db.h"
#include "common/macros.h"
#include "common/text_file.h"
#include "test_util.h"

using std::string;
using std::vector;
using namespace sicxe::assembler;

namespace sicxe {
namespace tests {

// A simple CodeOutputWriter that dumps everything it gets from the CodeGenerator to a
// TextFile.
class TestOutputWriter : public CodeOutputWriter {
 public:
  DISALLOW_COPY_AND_MOVE(TestOutputWriter);

  explicit TestOutputWriter(TextFile* text_file) : text_file_(text_file) {};
  ~TestOutputWriter() {}

  virtual void Begin(const Code&) {
    line_ = "Begin";
    WriteLine();
  }

  virtual void WriteNode(const node::Node&, uint32 address) {
    line_ = "Node-E:";
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "%06x", address);
    line_ += string(buffer);
    WriteLine();
  }

  virtual void WriteNode(const node::Node&, uint32 address,
                         const std::string& data, bool can_split) {
    if (can_split) {
      line_ = "Node-S:";
    } else {
      line_ = "Node-T:";
    }
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "%06x >>> ", address);
    line_ += string(buffer);
    for (size_t i = 0; i < data.size(); i++) {
      snprintf(buffer, sizeof(buffer), "%02x", static_cast<uint32>(data[i]) & 0xff);
      line_ += string(buffer);
    }
    WriteLine();
  }

  virtual void WriteRelocationRecord(const RelocationRecord& record) {
    if (record.type == RelocationRecord::SIMPLE) {
      line_ = "RRec-S:";
    } else {
      line_ = "RRec-N:";
    }
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "%06x [%d]", record.address, record.nibbles);
    line_ += string(buffer);
    if (record.type == RelocationRecord::SYMBOL) {
      line_ += " ";
      line_ += record.sign ? "+" : "-";
      line_ += record.symbol_name;
    }
    WriteLine();
  }

  virtual void End() {
    line_ = "End";
    WriteLine();
  }

 private:
  void WriteLine() {
    text_file_->mutable_lines()->emplace_back(std::move(line_));
    line_ = string();
  }

  TextFile* text_file_;
  string line_;
};

struct CodeGeneratorTestParams {
  CodeGeneratorTestParams(int in, bool s)
    : input_number(in), success(s) {}

  int input_number;
  bool success;
};

std::ostream& operator<<(std::ostream& stream, const CodeGeneratorTestParams& params) {
  stream << "[number=" << params.input_number << "; success=" << params.success << "]";
  return stream;
}

class CodeGeneratorTest : public testing::TestWithParam<CodeGeneratorTestParams> {
 protected:
  CodeGeneratorTest() {
    config_.instruction_db = InstructionDB::Default();
  }

  Parser::Config config_;
  TextFile input_file_;
  TextFile output_file_;
  TextFile error_file_;
  TextFile log_file_;
  TextFile expected_file_;
  TextFile expected_log_file_;
  TextFile expected_error_file_;
  Code code_;
  ErrorDB error_db_;
};

TEST_P(CodeGeneratorTest, GenerateCode) {
  char buffer[200];
  const CodeGeneratorTestParams& params = GetParam();
  config_.case_sensitive = true;
  config_.allow_brackets = true;
  if (params.success) {
    snprintf(buffer, sizeof(buffer), "codegen-in-%d-good.asm", params.input_number);
  } else {
    snprintf(buffer, sizeof(buffer), "codegen-in-%d-bad.asm", params.input_number);
  }
  ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &input_file_));
  Parser p(&config_);
  ASSERT_TRUE(p.ParseFile(input_file_, &code_, &error_db_));
  TableBuilder tb;
  ASSERT_TRUE(tb.BuildTables(&code_, &error_db_));
  CodeGenerator::OutputWriterVector writers;
  LogFileWriter log_writer(InstructionDB::Default(), &log_file_);
  writers.push_back(&log_writer);
  TestOutputWriter test_writer(&output_file_);
  writers.push_back(&test_writer);
  CodeGenerator code_generator;
  if (params.success) {
    ASSERT_TRUE(code_generator.GenerateCode(code_, &writers, &error_db_));
    snprintf(buffer, sizeof(buffer), "codegen-out-%d-good.txt", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &expected_file_));
    snprintf(buffer, sizeof(buffer), "codegen-log-%d-good.txt", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &expected_log_file_));
    EXPECT_PRED_FORMAT2(TestUtil::FilesEqual, expected_file_, output_file_);
    ASSERT_PRED_FORMAT2(TestUtil::FilesEqual, expected_log_file_, log_file_);
  } else {
    ASSERT_FALSE(code_generator.GenerateCode(code_, &writers, &error_db_));
    snprintf(buffer, sizeof(buffer), "codegen-err-%d-bad.txt", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &expected_error_file_));
    TestUtil::ErrorDBToTextFile(error_db_, &error_file_);
    ASSERT_PRED_FORMAT2(TestUtil::FilesEqual, expected_error_file_, error_file_);
  }
}

namespace {

vector<CodeGeneratorTestParams> GetInputs(int from, int to, bool success) {
  vector<CodeGeneratorTestParams> inputs;
  for (int i = from; i <= to; i++) {
    inputs.push_back(CodeGeneratorTestParams(i, success));
  }
  return inputs;
}

}  // namespace

INSTANTIATE_TEST_CASE_P(Good, CodeGeneratorTest,
                        ::testing::ValuesIn(GetInputs(1, 2, true)));
INSTANTIATE_TEST_CASE_P(Bad, CodeGeneratorTest,
                        ::testing::ValuesIn(GetInputs(1, 10, false)));

}  // namespace tests
}  // namespace sicxe
