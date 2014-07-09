#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "common/error_db.h"
#include "common/error_formatter.h"
#include "common/flags_parser.h"
#include "common/object_file.h"
#include "linker/linker.h"

using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {
namespace linker {

const char* kHelpMessage =
"SIC/XE Linker v1.0.0 by Klemen Kloboves\n"
"\n"
"Usage:    sicld [-h] [-p] [-c] [-a start_address] -o output_file files...\n"
"\n"
"Options:\n"
"\n"
"    -o, --output  output_file\n"
"        Write output to output_file.\n"
"\n"
"    -a, --start-address  start_address\n"
"        Specify output object file start address.\n"
"\n"
"    -p, --partial-link\n"
"        Undefined symbols do not result in an error but are written out to\n"
"        output object file.\n"
"\n"
"    -c, --compatibility-mode\n"
"        Less strict on input file modification records, but output file will\n"
"        not be relocatable - without modification records.\n"
"\n"
"    -h, --help\n"
"        Display help.\n"
"\n"
;

class LinkerDriver {
 public:
  DISALLOW_COPY_AND_MOVE(LinkerDriver);

  LinkerDriver() {
    error_formatter_.set_application_name("sicld");
    flag_output_file_ = flags_parser_.AddFlagString("o", "output");
    flag_help_ = flags_parser_.AddFlagBool("h", "help");
    flag_address_ = flags_parser_.AddFlagString("a", "start-address");
    flag_partial_link_ = flags_parser_.AddFlagBool("p", "partial-link");
    flag_comptibility_mode_ = flags_parser_.AddFlagBool("c", "compatibility-mode");
  }

  int Main(int argc, char* argv[]) {
    bool success = RealMain(argc, argv);
    error_formatter_.PrintErrors(error_db_);
    return success ? 0 : 1;
  }

 private:
  bool RealMain(int argc, char* argv[]) {
    if (!flags_parser_.ParseFlags(argc, argv, &error_db_)) {
      return false;
    }

    if (argc == 1 || flag_help_->value_bool) {
      PrintHelp();
      return true;
    }

    if (flags_parser_.args().empty()) {
      error_db_.AddError(ErrorDB::ERROR, "no input files", nullptr);
      return false;
    }
    if (!flag_output_file_->is_set) {
      error_db_.AddError(ErrorDB::ERROR, "output file not specified", nullptr);
      return false;
    }

    uint32 start_address = 0;
    if (flag_address_->is_set) {
      const char* address_str = flag_address_->value_string.c_str();
      char* end_ptr;
      errno = 0;
      uint64 value = static_cast<uint64>(strtoull(address_str, &end_ptr, 0));
      if (*end_ptr != '\0') {
        error_db_.AddError(ErrorDB::ERROR, "start address must be a number", nullptr);
        return false;
      }
      if (errno == ERANGE || value > 0x100000) {
        error_db_.AddError(ErrorDB::ERROR, "start address too large", nullptr);
        return false;
      }
      start_address = value;
    }

    Linker::ObjectFileVector input_ptrs;
    bool success = true;
    for (const string& file_name : flags_parser_.args()) {
      unique_ptr<ObjectFile> file(new ObjectFile);
      ObjectFile::LoadResult result = file->LoadFile(file_name.c_str());
      if (result == ObjectFile::OPEN_FAILED) {
        string message = "cannot open file '" + file_name + "'";
        error_db_.AddError(ErrorDB::ERROR, message.c_str(), nullptr);
        success = false;
        continue;
      } else if (result == ObjectFile::INVALID_FORMAT) {
        string message = "invalid object file '" + file_name + "'";
        error_db_.AddError(ErrorDB::ERROR, message.c_str(), nullptr);
        success = false;
        continue;
      }
      input_ptrs.push_back(file.get());
      input_files_.emplace_back(std::move(file));
    }
    if (!success) {
      return false;
    }

    Linker::Config config;
    config.compatibility_mode = flag_comptibility_mode_->value_bool;
    Linker linker(&config);
    if (!linker.LinkFiles(flag_partial_link_->value_bool, start_address,
                          input_ptrs, &output_file_, &error_db_)) {
      return false;
    }

    if (!output_file_.SaveFile(flag_output_file_->value_string.c_str())) {
      string message = "cannot write file '" + flag_output_file_->value_string + "'";
      error_db_.AddError(ErrorDB::ERROR, message.c_str(), nullptr);
      return false;
    }
    return true;
  }

  void PrintHelp() {
    printf("%s\n", kHelpMessage);
  }

  ErrorDB error_db_;
  ErrorFormatter error_formatter_;
  FlagsParser flags_parser_;
  const FlagsParser::Flag* flag_output_file_;
  const FlagsParser::Flag* flag_help_;
  const FlagsParser::Flag* flag_address_;
  const FlagsParser::Flag* flag_partial_link_;
  const FlagsParser::Flag* flag_comptibility_mode_;
  vector<unique_ptr<ObjectFile> > input_files_;
  ObjectFile output_file_;
};

}  // namespace linker
}  // namespace sicxe

int main(int argc, char* argv[]) {
  return sicxe::linker::LinkerDriver().Main(argc, argv);
}
