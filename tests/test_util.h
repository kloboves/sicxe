#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "common/error_db.h"
#include "common/macros.h"
#include "common/text_file.h"

namespace sicxe {
namespace tests {

class TestUtil {
 public:
  DISALLOW_INSTANTIATE(TestUtil);

  static std::string ConcatStrVector(const std::vector<std::string>& v,
                                     const std::string& separator);

  static std::string ErrorDBToStringSimple(const ErrorDB& error_db);
  static void ErrorDBToTextFile(const ErrorDB& error_db, TextFile* text_file);

  static std::string TestDataFileName(const char* file_name);
  static bool LoadTestTextFile(const char* file_name, TextFile* file);

  static bool LoadFileToString(const char* file_name, std::string* contents);
  static bool LoadTestFileToString(const char* file_name, std::string* contents);

  static ::testing::AssertionResult FilesEqual(const char* a_expr, const char* b_expr,
                                               const TextFile& a, const TextFile& b);
};

}  // namespace tests
}  // namespace sicxe

#endif  // TEST_UTIL_H
