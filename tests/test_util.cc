#include "test_util.h"

#include <memory>
#include <gtest/gtest.h>

using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {
namespace tests {

string TestUtil::ConcatStrVector(const vector<string>& v, const string& separator) {
  string rv;
  for (auto it = v.begin(); it != v.end(); it++) {
    if (it != v.begin()) {
      rv.append(separator);
    }
    rv.append(*it);
  }
  return rv;
}

string TestUtil::ErrorDBToStringSimple(const ErrorDB& error_db) {
  string rv;
  unique_ptr<char> buffer(new char[100]);
  for (const auto& entry : error_db.entries()) {
    const char* severity_str = nullptr;
    switch (entry->severity) {
      case ErrorDB::ERROR:
        severity_str = "E";
        break;
      case ErrorDB::WARNING:
        severity_str = "W";
        break;
      case ErrorDB::INFO:
        severity_str = "I";
        break;
    }
    if (entry->file == nullptr || !entry->has_position) {
      snprintf(buffer.get(), 100, "%s;", severity_str);
    } else {
      snprintf(buffer.get(), 100, "%s[%d:%d:%d];", severity_str,
               entry->position.row + 1, entry->position.column + 1, entry->position.size);
    }
    rv += string(buffer.get());

  }
  return rv;
}

void TestUtil::ErrorDBToTextFile(const ErrorDB& error_db, TextFile* text_file) {
  text_file->set_file_name("errors.txt");
  unique_ptr<char> buffer(new char[100]);
  for (const auto& entry : error_db.entries()) {
    const char* severity_str = nullptr;
    switch (entry->severity) {
      case ErrorDB::ERROR:
        severity_str = "E";
        break;
      case ErrorDB::WARNING:
        severity_str = "W";
        break;
      case ErrorDB::INFO:
        severity_str = "I";
        break;
    }
    if (entry->file == nullptr || !entry->has_position) {
      snprintf(buffer.get(), 100, "%s - ", severity_str);
    } else {
      snprintf(buffer.get(), 100, "%s[%d:%d:%d] - ", severity_str,
               entry->position.row + 1, entry->position.column + 1, entry->position.size);
    }
    text_file->mutable_lines()->emplace_back(string(buffer.get()) + entry->message);
  }
}

std::string TestUtil::TestDataFileName(const char* file_name) {
  string actual_file_name = "testdata/";
  actual_file_name += file_name;
  return actual_file_name;
}

bool TestUtil::LoadTestTextFile(const char* file_name, TextFile* file) {
  return file->Open(TestDataFileName(file_name));
}

bool TestUtil::LoadFileToString(const char* file_name, std::string* contents) {
  FILE* f = fopen(file_name, "r");
  if (f == nullptr) {
    return false;
  }
  *contents = string();
  const size_t buffer_size = 4096;
  unique_ptr<char[]> buffer(new char[buffer_size]);
  while (!feof(f)) {
    size_t size = fread(buffer.get(), 1, buffer_size, f);
    if (ferror(f)) {
      fclose(f);
      return false;
    }
    if (size > 0) {
      *contents += string(buffer.get(), size);
    }
  }
  fclose(f);
  return true;
}

bool TestUtil::LoadTestFileToString(const char* file_name, std::string* contents) {
  return LoadFileToString(TestDataFileName(file_name).c_str(), contents);
}

namespace {

string StripWhitespace(const string& str) {
  string rv = str;
  size_t len = 0;
  for (int i = rv.size() - 1; i >= 0; i--) {
    if (rv[i] != ' ' && rv[i] != '\t' && rv[i] != '\n') {
      len = i + 1;
      break;
    }
  }
  rv.resize(len, 0x00);
  return rv;
}

}  // namespace

::testing::AssertionResult TestUtil::FilesEqual(const char* a_expr, const char* b_expr,
                                                const TextFile& a, const TextFile& b) {
  bool equals = false;
  if (a.lines().size() == b.lines().size()) {
    equals = true;
    for (size_t i = 0; i < a.lines().size(); i++) {
      if (StripWhitespace(a.lines()[i]) != StripWhitespace(b.lines()[i])) {
        equals = false;
        break;
      }
    }
  }
  if (equals) {
    return ::testing::AssertionSuccess();
  }
  ::testing::AssertionResult result = ::testing::AssertionFailure();
  result << "Files " << a_expr << " and " << b_expr
         << " ('" << a.file_name() << "' and '" << b.file_name() << "') are different!";
  result << std::endl;
  result << "Contents of " << a_expr << " ('" << a.file_name() << "'):";
  result << std::endl << "---------------------------------------" << std::endl;
  for (const auto& line : a.lines()) {
    result << line << std::endl;
  }
  result << "---------------------------------------" << std::endl;
  result << "Contents of " << b_expr << " ('" << b.file_name() << "'):";
  result << std::endl << "---------------------------------------" << std::endl;
  for (const auto& line : b.lines()) {
    result << line << std::endl;
  }
  result << "---------------------------------------";
  return result;
}

}  // namespace tests
}  // namespace sicxe
