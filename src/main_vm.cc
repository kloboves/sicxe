#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include "common/error_db.h"
#include "common/error_formatter.h"
#include "common/flags_parser.h"
#include "common/object_file.h"
#include "machine/execute_result.h"
#include "machine/file_device.h"
#include "machine/io_device.h"
#include "machine/loader.h"
#include "machine/machine.h"

using std::string;
using std::unique_ptr;

namespace sicxe {
namespace machine {

const char* kHelpMessage =
"SIC/XE Virtual Machine v1.0.0 by Klemen Kloboves\n"
"\n"
"Usage:    sicvm [-h] object_file\n"
"\n"
"Options:\n"
"\n"
"    -h, --help\n"
"        Display help.\n"
"\n"
;

namespace {

volatile bool trigger_interrupt = false;

void Usr1SignalHandler(int) {
  trigger_interrupt = true;
}

}  // namespace

class VMDriver {
 public:
  DISALLOW_COPY_AND_MOVE(VMDriver);

  VMDriver() {
    error_formatter_.set_application_name("sicvm");
    flag_help_ = flags_parser_.AddFlagBool("h", "help");
    struct sigaction sa;
    sa.sa_handler = &Usr1SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    bool success = (sigaction(SIGUSR1, &sa, nullptr) == 0);
    assert(success);
    unused(success);
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
      error_db_.AddError(ErrorDB::ERROR, "no input object file", nullptr);
      return false;
    } else if (flags_parser_.args().size() > 1) {
      error_db_.AddError(ErrorDB::ERROR, "expected one input object file", nullptr);
      return false;
    }

    // open object file
    const string& object_file_name = flags_parser_.args().front();
    auto open_result = object_file_.LoadFile(object_file_name.c_str());
    if (open_result == ObjectFile::OPEN_FAILED) {
      string message = "cannot open file '" + object_file_name + "'";
      error_db_.AddError(ErrorDB::ERROR, message.c_str(), nullptr);
      return false;
    } else if (open_result == ObjectFile::INVALID_FORMAT) {
      string message = "invalid object file '" + object_file_name + "'";
      error_db_.AddError(ErrorDB::ERROR, message.c_str(), nullptr);
      return false;
    }

    // set up machine
    Machine machine;
    machine.SetDevice(0, new InputDevice);
    machine.SetDevice(1, new OutputDevice(false));
    machine.SetDevice(2, new OutputDevice(true));
    unique_ptr<char[]> device_name_buffer(new char[PATH_MAX]);
    for (int i = 3; i < 256; i++) {
      sprintf(device_name_buffer.get(), "%02X.dev", i);
      machine.SetDevice(i, new FileDevice(string(device_name_buffer.get())));
    }

    // load object file
    if (!MachineLoader::LoadObjectFile(object_file_, &machine)) {
      error_db_.AddError(ErrorDB::ERROR, "object file loading failed", nullptr);
      return false;
    }

    ExecuteResult::ResultId result = ExecuteResult::OK;
    while (true) {
      if (trigger_interrupt) {
        trigger_interrupt = false;
        machine.Interrupt();
      } else {
        result = machine.Execute();
        if (result != ExecuteResult::OK) {
          break;
        }
      }
    }

    if (result == ExecuteResult::ENDLESS_LOOP) {
      return true;
    }

    const char* error_str = nullptr;
    switch (result) {
      case ExecuteResult::DEVICE_ERROR:
        error_str = "device error";
        break;
      case ExecuteResult::INVALID_OPCODE:
        error_str = "invalid opcode";
        break;
      case ExecuteResult::INVALID_ADDRESSING:
        error_str = "invalid addressing";
        break;
      case ExecuteResult::NOT_IMPLEMENTED:
        error_str = "not implemented";
        break;
      default:
        error_str = "unknown machine error";
        break;
    }
    char error_buffer[100];
    snprintf(error_buffer, 100, "%s at 0x%06u", error_str,
             machine.cpu_state().program_counter);
    error_db_.AddError(ErrorDB::ERROR, error_buffer, nullptr);
    return false;
  }

  void PrintHelp() {
    printf("%s\n", kHelpMessage);
  }

  ErrorDB error_db_;
  ErrorFormatter error_formatter_;
  FlagsParser flags_parser_;
  const FlagsParser::Flag* flag_help_;
  ObjectFile object_file_;
};

}  // namespace machine
}  // namespace sicxe

int main(int argc, char* argv[]) {
  return sicxe::machine::VMDriver().Main(argc, argv);
}
