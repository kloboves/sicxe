#include "simulator/simulator.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <string>
#include <vector>
#include "common/instruction_db.h"
#include "machine/file_device.h"
#include "machine/logic_db.h"

using sicxe::machine::FileDevice;
using sicxe::machine::LogicDB;
using std::make_pair;
using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {
namespace simulator {

Simulator::Simulator() : Simulator(InstructionDB::Default(), LogicDB::Default()) {}

Simulator::Simulator(const InstructionDB* instruction_db, const LogicDB* logic_db)
  : instruction_db_(instruction_db), logic_db_(logic_db),
    machine_(instruction_db_, logic_db_), command_interface_("sicsim"),
    auto_disassemble_(false), breakpoints_enable_(false),
    breakpoint_next_number_(0), breakpoint_last_hit_address_(0xFFFFFF),
    variable_next_number_(0) {
  RegisterCommands();
  // set up devices
  unique_ptr<char[]> device_name_buffer(new char[PATH_MAX]);
  for (int i = 0; i < 256; i++) {
    sprintf(device_name_buffer.get(), "%02X.dev", i);
    machine_.SetDevice(i, new FileDevice(string(device_name_buffer.get())));
  }
}

Simulator::~Simulator() {}

void Simulator::Run() {
  command_interface_.Run();
}

void Simulator::RegisterCommands() {
  using std::placeholders::_1;

  command_interface_.RegisterCommand(
      vector<string>{"reset"},
      "Reset machine state (clear registers, memory).",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandReset, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"cpu", "print"},
      "Print the CPU state (all registers).",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandCpuPrint, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"cpu", "set"},
      "Set value of CPU register. Parameter 'register' should be PC, CC,\n"
      "A, X, L, B, S, T or F. Possible values for CC are LT, EQ or GT.",
      vector<pair<string, bool> >{
        make_pair("register", true),
        make_pair("value", true),
      },
      std::bind(&Simulator::CommandCpuSet, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"memory", "write", "byte"},
      "Write a byte to the specified memory address.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("value", true),
      },
      std::bind(&Simulator::CommandMemoryWriteByte, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "write", "word"},
      "Write a word (3 bytes) to the specified memory address.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("value", true),
      },
      std::bind(&Simulator::CommandMemoryWriteWord, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "write", "float"},
      "Write a float (6 bytes) to the specified memory address.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("value", true),
      },
      std::bind(&Simulator::CommandMemoryWriteFloat, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "write", "ascii"},
      "Write an ASCII string at the specified address. Null-terminated by\n"
      "default, can be disabled by setting terminate to false.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("value", true),
        make_pair("terminate", false),
      },
      std::bind(&Simulator::CommandMemoryWriteAscii, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "print", "bytes"},
      "Print out memory interpreted as bytes.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("count", false),
      },
      std::bind(&Simulator::CommandMemoryPrintBytes, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "print", "words"},
      "Print out memory interpreted as words.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("count", false),
      },
      std::bind(&Simulator::CommandMemoryPrintWords, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "print", "floats"},
      "Print out memory interpreted as floating point numbers.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("count", false),
      },
      std::bind(&Simulator::CommandMemoryPrintFloats, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"load"},
      "Reset the machine and load an object file to the specified address.",
      vector<pair<string, bool> >{
        make_pair("file", true),
        make_pair("address", false),
      },
      std::bind(&Simulator::CommandLoad, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"disassembler", "auto", "on"},
      "Enable automatic disassembly when stepping.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandDisassembleAutoOn, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"disassembler", "auto", "off"},
      "Disable automatic disassembly when stepping.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandDisassembleAutoOff, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"disassembler", "print"},
      "Disassemble instructions at a specified address.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("count", false),
      },
      std::bind(&Simulator::CommandDisassemble, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"start"},
      "Run the machine until an error or breakpoint is encountered (only\n"
      "if breakpoints are enabled). Can also be stopped by pressing Ctrl+C.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandStart, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"step"},
      "Execute up to count instructions (default is 1). The machine\n"
      "stops if an error or breakpoint is encountered (if enabled). Also\n"
      "shows disassembly of executed instructions if enabled in the\n"
      "disassembly menu.",
      vector<pair<string, bool> >{
        make_pair("count", false),
      },
      std::bind(&Simulator::CommandStep, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"interrupt"},
      "Trigger an interrupt (no effect if interrupts are disabled).",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandInterrupt, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"breakpoint", "on"},
      "Enable breakpoints.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandBreakpointOn, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"breakpoint", "off"},
      "Disable breakpoints.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandBreakpointOff, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"breakpoint", "clear"},
      "Remove all breakpoints.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandBreakpointClear, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"breakpoint", "print"},
      "Print list of breakpoints.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandBreakpointPrint, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"breakpoint", "add"},
      "Add a breakpoint.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("name", false),
      },
      std::bind(&Simulator::CommandBreakpointAdd, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"breakpoint", "remove"},
      "Remove a breakpoint (specify only 'number', 'address' or 'name').",
      vector<pair<string, bool> >{
        make_pair("number", false),
        make_pair("address", false),
        make_pair("name", false),
      },
      std::bind(&Simulator::CommandBreakpointRemove, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"watchlist", "print"},
      "Print variable watch list.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandWatchlistPrint, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"watchlist", "clear"},
      "Clear variable watch list.",
      vector<pair<string, bool> >{},
      std::bind(&Simulator::CommandWatchlistClear, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"watchlist", "add"},
      "Add a variable to the watch list.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("type", true),
        make_pair("name", false),
      },
      std::bind(&Simulator::CommandWatchlistAdd, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"watchlist", "remove"},
      "Remove a variable from the watch list (specify either 'number' or 'name').",
      vector<pair<string, bool> >{
        make_pair("number", false),
        make_pair("name", false),
      },
      std::bind(&Simulator::CommandWatchlistRemove, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"watchlist", "write"},
      "Set net value to a variable on the watch list.",
      vector<pair<string, bool> >{
        make_pair("number", false),
        make_pair("name", false),
        make_pair("value", true),
      },
      std::bind(&Simulator::CommandWatchlistWrite, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"device", "reset"},
      "Reset a device with specified number.",
      vector<pair<string, bool> >{
        make_pair("number", true),
      },
      std::bind(&Simulator::CommandDeviceReset, this, _1));
}

string Simulator::ConvertToUppercase(const string& str) const {
  string result = str;
  for (size_t i = 0; i < result.size(); i++) {
    if (isalpha(result[i])) {
      result[i] = toupper(result[i]);
    }
  }
  return result;
}

void Simulator::CommandReset(const CommandInterface::ParsedArgumentMap&) {
  machine_.Reset();
}

}  // namespace simulator
}  // namespace sicxe
