#include "fpga/utility.h"

#include <limits.h>
#include <stdlib.h>
#include <map>
#include <memory>
#include <utility>
#include <vector>
#include "common/object_file.h"
#include "fpga/loader.h"

using std::make_pair;
using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {
namespace fpga {

FpgaUtility::FpgaUtility(const std::string& device_file_name)
  : command_interface_("sicfpga"), connection_(device_file_name) {
  RegisterCommands();
}

FpgaUtility::~FpgaUtility() {}

void FpgaUtility::Run() {
  if (!connection_.Open()) {
    printf("Error: Could not open device\n");
    return;
  }
  if (!connection_.TestConnection()) {
    printf("Error: Connection failed. Make sure the device is connected.\n");
    return;
  }
  command_interface_.Run();
}

void FpgaUtility::RegisterCommands() {
  using std::placeholders::_1;

  // register all simulator commands
  command_interface_.RegisterCommand(
      vector<string>{"memory", "write", "byte"},
      "Write a byte to device memory.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("value", true),
      },
      std::bind(&FpgaUtility::CommandMemoryWriteByte, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "write", "word"},
      "Write a word (3 bytes) to device memory.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("value", true),
      },
      std::bind(&FpgaUtility::CommandMemoryWriteWord, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "write", "ascii"},
      "Write an ASCII string to device memory. Null-terminated by\n"
      "default, can be disabled by setting 'terminate' to false.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("value", true),
        make_pair("terminate", false),
      },
      std::bind(&FpgaUtility::CommandMemoryWriteAscii, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "print", "bytes"},
      "Read device memory and print as bytes.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("count", false),
      },
      std::bind(&FpgaUtility::CommandMemoryPrintBytes, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"memory", "print", "words"},
      "Read device memory and print as words.",
      vector<pair<string, bool> >{
        make_pair("address", true),
        make_pair("count", false),
      },
      std::bind(&FpgaUtility::CommandMemoryPrintWords, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"load"},
      "Stop and reset CPU, load object file to device memory.",
      vector<pair<string, bool> >{
        make_pair("file", true),
      },
      std::bind(&FpgaUtility::CommandLoad, this, _1));

  command_interface_.RegisterCommand(
      vector<string>{"reset"},
      "Reset CPU.",
      vector<pair<string, bool> >(),
      std::bind(&FpgaUtility::CommandReset, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"start"},
      "Start CPU.",
      vector<pair<string, bool> >(),
      std::bind(&FpgaUtility::CommandStart, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"stop"},
      "Stop CPU.",
      vector<pair<string, bool> >(),
      std::bind(&FpgaUtility::CommandStop, this, _1));
  command_interface_.RegisterCommand(
      vector<string>{"interrupt"},
      "Trigger an interrupt.",
      vector<pair<string, bool> >(),
      std::bind(&FpgaUtility::CommandInterrupt, this, _1));
}

const int FpgaUtility::kMemoryPrintDefaultByteCount = 8 * 16;
const int FpgaUtility::kMemoryPrintDefaultWordCount = 10;
const int FpgaUtility::kMemoryPrintMaxByteCount = 1000 * 16;
const int FpgaUtility::kMemoryPrintMaxWordCount = 1000;

void FpgaUtility::CommandMemoryWriteByte(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  uint8 value = 0;
  if (value_arg.value_str.size() == 3 &&
      value_arg.value_str[0] == '\'' &&
      value_arg.value_str[2] == '\'') {
    value = static_cast<uint8>(value_arg.value_str[1]);
  } else {
    if (!value_arg.is_word) {
      printf("Error: Value must be a number or ASCII character in single quotes!\n");
      return;
    }
    value = value_arg.value_word & 0xFF;
  }
  if (!connection_.WriteMemoryByte(address, value)) {
    printf("Error: Write failed!\n");
  }
}

void FpgaUtility::CommandMemoryWriteWord(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  if (!value_arg.is_word) {
    printf("Error: Value must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  uint32 value = value_arg.value_word;
  if (!connection_.WriteMemoryWord(address, value)) {
    printf("Error: Write failed!\n");
  }
}

void FpgaUtility::CommandMemoryWriteAscii(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  const CommandInterface::ParsedArgument& value_arg = arguments.find("value")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  bool null_terminate = true;
  {
    auto it = arguments.find("terminate");
    if (it != arguments.end()) {
      const CommandInterface::ParsedArgument& null_term_arg = it->second;
      if (!null_term_arg.is_bool) {
        printf("Error: Terminate must be a boolean!\n");
        return;
      }
      null_terminate = null_term_arg.value_bool;
    }
  }
  bool write_success = false;
  if (null_terminate) {
    write_success = connection_.WriteMemory(
        address, value_arg.value_str.size() + 1,
        reinterpret_cast<const uint8*>(value_arg.value_str.c_str()));
  } else {
    write_success = connection_.WriteMemory(
        address, value_arg.value_str.size(),
        reinterpret_cast<const uint8*>(value_arg.value_str.data()));
  }
  if (!write_success) {
    printf("Error: Write failed!\n");
  }
}

void FpgaUtility::CommandMemoryPrintBytes(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  int count = kMemoryPrintDefaultByteCount;
  {
    auto it = arguments.find("count");
    if (it != arguments.end()) {
      const CommandInterface::ParsedArgument& count_arg = it->second;
      if (!count_arg.is_word) {
        printf("Error: Count must be a number!\n");
        return;
      }
      count = count_arg.value_word;
    }
  }
  if (count > kMemoryPrintMaxByteCount) {
    printf("Error: Count must be less than or equal to %d!\n", kMemoryPrintMaxByteCount);
    return;
  }
  if (address + count > (0x0FFFFF + 1)) {
    count = 0x0FFFFF - address + 1;
  }
  if (count <= 0) {
    return;
  }
  unique_ptr<uint8[]> data_buffer(new uint8[count]);
  if (!connection_.ReadMemory(address, count, data_buffer.get())) {
    printf("Error: Read failed!\n");
    return;
  }
  uint32 row_address = (address / 16) * 16;
  for (; row_address < address + count; row_address += 16) {
    printf(" %06x", row_address);
    for (int i = 0; i < 16; i++) {
      if (i == 0 || i == 8) {
        printf(" ");
      }
      printf(" ");
      uint32 a = row_address + i;
      if (a >= address && a < address + count) {
        printf("%02x", static_cast<uint32>(data_buffer[a - address]));
      } else {
        printf("  ");
      }
    }

    printf("  |");
    for (int i = 0; i < 16; i++) {
      uint32 a = row_address + i;
      if (a >= address && a < address + count) {
        uint8 value = data_buffer[a - address];
        if (value >= 0x20 && value <= 0x7E) {
          printf("%c", value);
        } else {
          printf(".");
        }
      } else {
        printf(" ");
      }
    }
    printf("|\n");
  }
}

void FpgaUtility::CommandMemoryPrintWords(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& address_arg = arguments.find("address")->second;
  if (!address_arg.is_word) {
    printf("Error: Address must be a number!\n");
    return;
  }
  uint32 address = address_arg.value_word & 0x0FFFFF;
  int count = kMemoryPrintDefaultWordCount;
  {
    auto it = arguments.find("count");
    if (it != arguments.end()) {
      const CommandInterface::ParsedArgument& count_arg = it->second;
      if (!count_arg.is_word) {
        printf("Error: Count must be a number!\n");
        return;
      }
      count = count_arg.value_word;
    }
  }
  if (count > kMemoryPrintMaxWordCount) {
    printf("Error: Count must be less than or equal to %d!\n", kMemoryPrintMaxWordCount);
    return;
  }
  if (address + 3 * count > 0x0FFFFF) {
    count = (0x0FFFFF - address + 1) / 3;
  }
  if (count <= 0) {
    return;
  }
  printf(" %-12s %-12s %-12s %-12s\n",
         "ADDRESS", "HEX", "UNSIGNED", "SIGNED");
  unique_ptr<uint8[]> data_buffer(new uint8[3 * count]);
  if (!connection_.ReadMemory(address, 3 * count, data_buffer.get())) {
    printf("Error: Read failed!\n");
    return;
  }
  uint32 word_address = address;
  for (int i = 0; i < count; i++, word_address += 3) {
    uint32 value = 0;
    value = data_buffer[word_address - address];
    value <<= 8;
    value |= data_buffer[word_address - address + 1];
    value <<= 8;
    value |= data_buffer[word_address - address + 2];
    uint32 temp = 0xFF000000;
    if (((value >> 23) & 0x1) == 0x0) {
      temp = 0;
    }
    temp |= value;
    int32 value_signed = static_cast<int32>(temp);
    printf(" %06x%-6s %06x%-6s %-12u %-12d\n", word_address, "",
           value, "", value, value_signed);
  }
}

void FpgaUtility::CommandLoad(const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& filename_arg = arguments.find("file")->second;
  unique_ptr<char[]> real_filename(new char[PATH_MAX]);
  if (realpath(filename_arg.value_str.c_str(), real_filename.get()) == nullptr) {
    printf("Error: Could not resolve filename!\n");
    return;
  }
  ObjectFile object_file;
  ObjectFile::LoadResult rv = object_file.LoadFile(real_filename.get());
  if (rv == ObjectFile::OPEN_FAILED) {
    printf("Error: Could not open file!\n");
    return;
  }
  if (rv == ObjectFile::INVALID_FORMAT) {
    printf("Error: Invalid file format!\n");
    return;
  }
  if (rv != ObjectFile::OK) {
    printf("Error: Unknown error occured when opening object file!\n");
    return;
  }
  if (object_file.start_address() != 0) {
    printf("Error: Object file start at address must be 0!\n");
    return;
  }
  if (object_file.entry_point() != 0) {
    printf("Error: Object file entry point must be 0!\n");
    return;
  }
  // stop and reset CPU
  if (!connection_.ControlSignal(2) ||
      !connection_.ControlSignal(0)) {
    printf("Error: Failed to send control signal!\n");
    return;
  }
  if (!FpgaLoader::LoadObjectFile(object_file, &connection_)) {
    printf("Error: Loading failed!\n");
    return;
  }
}

void FpgaUtility::CommandReset(const CommandInterface::ParsedArgumentMap&) {
  if (!connection_.ControlSignal(0)) {
    printf("Error: Failed to send control signal!\n");
    return;
  }
}

void FpgaUtility::CommandStart(const CommandInterface::ParsedArgumentMap&) {
  if (!connection_.ControlSignal(1)) {
    printf("Error: Failed to send control signal!\n");
    return;
  }
}

void FpgaUtility::CommandStop(const CommandInterface::ParsedArgumentMap&) {
  if (!connection_.ControlSignal(2)) {
    printf("Error: Failed to send control signal!\n");
    return;
  }
}

void FpgaUtility::CommandInterrupt(const CommandInterface::ParsedArgumentMap&) {
  if (!connection_.ControlSignal(3)) {
    printf("Error: Failed to send control signal!\n");
    return;
  }
}

}  // namespace fpga
}  // namespace sicxe
