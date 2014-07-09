#ifndef FPGA_UTILITY_H
#define FPGA_UTILITY_H

#include <string>
#include "common/command_interface.h"
#include "common/macros.h"
#include "common/types.h"
#include "fpga/connection.h"

namespace sicxe {
namespace fpga {

class FpgaUtility {
 public:
  DISALLOW_COPY_AND_MOVE(FpgaUtility);

  explicit FpgaUtility(const std::string& device_file_name);
  ~FpgaUtility();

  void Run();

 private:
  static const int kMemoryPrintDefaultByteCount;
  static const int kMemoryPrintDefaultWordCount;
  static const int kMemoryPrintMaxByteCount;
  static const int kMemoryPrintMaxWordCount;

  void RegisterCommands();

  // memory commands
  void CommandMemoryWriteByte(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryWriteWord(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryWriteAscii(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryPrintBytes(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryPrintWords(const CommandInterface::ParsedArgumentMap& arguments);

  // load command
  void CommandLoad(const CommandInterface::ParsedArgumentMap& arguments);

  // control commands
  void CommandReset(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandStart(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandStop(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandInterrupt(const CommandInterface::ParsedArgumentMap& arguments);

  CommandInterface command_interface_;
  FpgaConnection connection_;
};

}  // namespace fpga
}  // namespace sicxe

#endif  // FPGA_UTILITY_H
