#include <stdio.h>
#include "common/error_db.h"
#include "common/error_formatter.h"
#include "common/flags_parser.h"
#include "fpga/utility.h"

namespace sicxe {
namespace fpga {

const char* kHelpMessage =
"SIC/XE FPGA Communicator v1.0.0 by Klemen Kloboves\n"
"\n"
"Usage:    sicfpga [-h] -d device_file\n"
"\n"
"Options:\n"
"\n"
"    -d, --device  device_file\n"
"        Specify device file of serial port connected to the FPGA board.\n"
"\n"
"    -h, --help\n"
"        Display help.\n"
"\n"
;

class FpgaUtilityDriver {
 public:
  DISALLOW_COPY_AND_MOVE(FpgaUtilityDriver);

  FpgaUtilityDriver() {
    error_formatter_.set_application_name("sicfpga");
    flag_help_ = flags_parser_.AddFlagBool("h", "help");
    flag_device_ = flags_parser_.AddFlagString("d", "device");
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

    if (flag_help_->value_bool) {
      PrintHelp();
      return true;
    }

    if (!flag_device_->is_set) {
      error_db_.AddError(ErrorDB::ERROR, "device not specified (option '-d')");
      return false;
    }
    if (!flags_parser_.args().empty()) {
      error_db_.AddError(ErrorDB::ERROR, "unexpected arguments");
      return false;
    }

    FpgaUtility utility(flag_device_->value_string);
    utility.Run();
    return true;
  }

  void PrintHelp() {
    printf("%s\n", kHelpMessage);
  }

  ErrorDB error_db_;
  ErrorFormatter error_formatter_;
  FlagsParser flags_parser_;
  const FlagsParser::Flag* flag_help_;
  const FlagsParser::Flag* flag_device_;
};

}  // namespace fpga
}  // namespace sicxe

int main(int argc, char* argv[]) {
  return sicxe::fpga::FpgaUtilityDriver().Main(argc, argv);
}
