#ifndef SIMULATOR_SIMULATOR_H
#define SIMULATOR_SIMULATOR_H

#include <list>
#include <map>
#include <string>
#include "common/command_interface.h"
#include "common/cpu_state.h"
#include "common/macros.h"
#include "common/types.h"
#include "machine/machine.h"

namespace sicxe {

class InstructionDB;
namespace machine { class LogicDB; }

namespace simulator {

class Simulator {
 public:
  DISALLOW_COPY_AND_MOVE(Simulator);

  Simulator();
  Simulator(const InstructionDB* instruction_db, const machine::LogicDB* logic_db);
  ~Simulator();

  void Run();

 private:
  struct Breakpoint {
    int number;
    uint32 address;
    std::string name;

    std::list<std::unique_ptr<Breakpoint> >::iterator list_iter;
    std::map<int, Breakpoint*>::iterator number_map_iter;
    std::map<uint32, Breakpoint*>::iterator address_map_iter;
    std::map<std::string, Breakpoint*>::iterator name_map_iter;
  };

  struct Variable {
    enum TypeId {
      BYTE = 0,
      WORD,
      ASCII,
      FLOAT
    };

    int number;
    uint32 address;
    std::string name;
    TypeId type;

    std::list<std::unique_ptr<Variable> >::iterator list_iter;
    std::map<int, Variable*>::iterator number_map_iter;
    std::map<std::string, Variable*>::iterator name_map_iter;
  };

  static const int kMemoryPrintDefaultByteCount;
  static const int kMemoryPrintDefaultWordCount;
  static const int kMemoryPrintDefaultFloatCount;
  static const int kMemoryPrintMaxByteCount;
  static const int kMemoryPrintMaxWordCount;
  static const int kMemoryPrintMaxFloatCount;
  static const int kDisassembleDefaultCount;
  static const int kDisassembleMaxCount;
  static const int kStepMaxCount;
  static const int kVariableNameMaxLength;

  void RegisterCommands();

  std::string ConvertToUppercase(const std::string& str) const;
  // returns 0 if disassembly failed, otherwise returns parsed instruction length
  int DisassembleInstruction(uint32 address);
  Variable* FindVariable(const CommandInterface::ParsedArgumentMap& arguments);

  void CommandReset(const CommandInterface::ParsedArgumentMap& arguments);

  // cpu commands
  void CommandCpuPrint(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandCpuSet(const CommandInterface::ParsedArgumentMap& arguments);

  // memory commands
  void CommandMemoryWriteByte(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryWriteWord(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryWriteFloat(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryWriteAscii(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryPrintBytes(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryPrintWords(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandMemoryPrintFloats(const CommandInterface::ParsedArgumentMap& arguments);

  // load command
  void CommandLoad(const CommandInterface::ParsedArgumentMap& arguments);

  // disassembler commands
  void CommandDisassembleAutoOn(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandDisassembleAutoOff(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandDisassemble(const CommandInterface::ParsedArgumentMap& arguments);

  // machine control commands
  void CommandStep(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandStart(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandInterrupt(const CommandInterface::ParsedArgumentMap& arguments);

  // breakpoint commands
  void CommandBreakpointOn(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandBreakpointOff(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandBreakpointClear(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandBreakpointPrint(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandBreakpointAdd(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandBreakpointRemove(const CommandInterface::ParsedArgumentMap& arguments);

  // variable watchlist commands
  void CommandWatchlistPrint(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandWatchlistClear(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandWatchlistAdd(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandWatchlistRemove(const CommandInterface::ParsedArgumentMap& arguments);
  void CommandWatchlistWrite(const CommandInterface::ParsedArgumentMap& arguments);

  // device commands
  void CommandDeviceReset(const CommandInterface::ParsedArgumentMap& arguments);

  const InstructionDB* instruction_db_;
  const machine::LogicDB* logic_db_;
  machine::Machine machine_;
  CommandInterface command_interface_;
  bool auto_disassemble_;

  bool breakpoints_enable_;
  std::list<std::unique_ptr<Breakpoint> > breakpoints_;
  std::map<int, Breakpoint*> breakpoint_number_map_;
  std::map<uint32, Breakpoint*> breakpoint_address_map_;
  std::map<std::string, Breakpoint*> breakpoint_name_map_;
  int breakpoint_next_number_;
  uint32 breakpoint_last_hit_address_;

  std::list<std::unique_ptr<Variable> > variables_;
  std::map<int, Variable*> variable_number_map_;
  std::map<std::string, Variable*> variable_name_map_;
  int variable_next_number_;
};

}  // namespace simulator
}  // namespace sicxe

#endif  // SIMULATOR_SIMULATOR_H
