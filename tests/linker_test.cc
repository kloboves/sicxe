#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>
#include "common/error_db.h"
#include "common/object_file.h"
#include "common/text_file.h"
#include "common/types.h"
#include "linker/linker.h"
#include "test_util.h"

using std::string;
using std::unique_ptr;
using std::vector;
using namespace sicxe::linker;

namespace sicxe {
namespace tests {

struct LinkerTestParams {
  LinkerTestParams(bool cm, int in, int f, bool pl, uint32 addr, bool s)
    : compatibility_mode(cm), input_number(in), number_files(f), partial_link(pl),
      start_address(addr), success(s) {}

  bool compatibility_mode;
  int input_number;
  int number_files;
  bool partial_link;
  uint32 start_address;
  bool success;
};

std::ostream& operator<<(std::ostream& stream, const LinkerTestParams& params) {
  stream << "[number=" << params.input_number
         << "; number_files=" << params.number_files
         << "; partial_link=" << params.partial_link
         << "; start_address=" << params.start_address
         << "; success=" << params.success << "]";
  return stream;
}

class LinkerTest : public testing::TestWithParam<LinkerTestParams> {
 protected:
  LinkerTest() {
  }

  vector<unique_ptr<ObjectFile> > input_files_;
  ObjectFile output_file_;
  ErrorDB error_db_;
};


TEST_P(LinkerTest, LinkFiles) {
  char buffer[200];
  const LinkerTestParams& params = GetParam();

  for (int i = 0; i < params.number_files; i++) {
    int file_number = i + 1;
    if (params.success) {
      snprintf(buffer, sizeof(buffer), "linker-in-%d-%d-good.obj",
               params.input_number, file_number);
    } else {
      snprintf(buffer, sizeof(buffer), "linker-in-%d-%d-bad.obj",
               params.input_number, file_number);
    }
    unique_ptr<ObjectFile> file(new ObjectFile);
    ASSERT_EQ(ObjectFile::OK, file->LoadFile(TestUtil::TestDataFileName(buffer).c_str()));
    input_files_.emplace_back(std::move(file));
  }

  vector<const ObjectFile*> input_file_ptrs;
  for (const auto& file : input_files_) {
    input_file_ptrs.push_back(file.get());
  }

  Linker::Config config;
  config.compatibility_mode = params.compatibility_mode;
  Linker linker(&config);
  if (params.success) {
    ASSERT_TRUE(linker.LinkFiles(params.partial_link, params.start_address,
                                 input_file_ptrs, &output_file_, &error_db_));
    string expected_file_str;
    snprintf(buffer, sizeof(buffer), "linker-out-%d-good.obj", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestFileToString(buffer, &expected_file_str));
    snprintf(buffer, sizeof(buffer), "testtemp-linker-%d.obj", params.input_number);
    output_file_.SaveFile(buffer);
    string output_file_str;
    ASSERT_TRUE(TestUtil::LoadFileToString(buffer, &output_file_str));
    ASSERT_EQ(expected_file_str, output_file_str);
  } else {
    ASSERT_FALSE(linker.LinkFiles(params.partial_link, params.start_address,
                                  input_file_ptrs, &output_file_, &error_db_));
    TextFile expected_file;
    snprintf(buffer, sizeof(buffer), "linker-err-%d-bad.txt", params.input_number);
    ASSERT_TRUE(TestUtil::LoadTestTextFile(buffer, &expected_file));
    TextFile error_file;
    TestUtil::ErrorDBToTextFile(error_db_, &error_file);
    ASSERT_PRED_FORMAT2(TestUtil::FilesEqual, expected_file, error_file);
  }
}

namespace {

vector<LinkerTestParams> GetGoodInputs() {
  vector<LinkerTestParams> inputs {
    LinkerTestParams(false, 1, 1, true, 1000, true),
    LinkerTestParams(false, 2, 4, false, 0, true),
    LinkerTestParams(false, 3, 2, false, 3000, true),
    LinkerTestParams(true, 4, 2, false, 0, true),
  };
  return inputs;
}

vector<LinkerTestParams> GetBadInputs() {
  vector<LinkerTestParams> inputs {
    LinkerTestParams(false, 1, 2, false, 0xfffc3, false),
    LinkerTestParams(false, 2, 2, false, 0x0, false),
    LinkerTestParams(false, 3, 1, false, 0x0, false),
    LinkerTestParams(false, 4, 3, false, 0x0, false),
    LinkerTestParams(false, 5, 1, false, 0x0, false),
    LinkerTestParams(false, 6, 1, false, 0x0, false),
    LinkerTestParams(false, 7, 1, false, 0x0, false),
    LinkerTestParams(false, 8, 2, false, 0x0, false),
    LinkerTestParams(false, 9, 2, false, 0x0, false),
    LinkerTestParams(false, 10, 2, false, 0x0, false),
    LinkerTestParams(true, 11, 0, true, 0x0, false),
    LinkerTestParams(false, 12, 2, false, 0x0, false),
  };
  return inputs;
}

}  // namespace

INSTANTIATE_TEST_CASE_P(Good, LinkerTest, ::testing::ValuesIn(GetGoodInputs()));
INSTANTIATE_TEST_CASE_P(Bad, LinkerTest, ::testing::ValuesIn(GetBadInputs()));

}  // namespace tests
}  // namespace sicxe
