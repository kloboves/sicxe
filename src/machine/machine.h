#ifndef MACHINE_MACHINE_H
#define MACHINE_MACHINE_H

#include <memory>
#include "common/cpu_state.h"
#include "common/macros.h"
#include "common/types.h"
#include "machine/execute_result.h"

namespace sicxe {

class InstructionDB;
struct InstructionInstance;

namespace machine {

class Device;
class LogicDB;

class Machine {
 public:
  DISALLOW_COPY_AND_MOVE(Machine);

  static const size_t kMemorySize;
  static int32 SignExtendWord(uint32 word);
  static uint32 TrimWord(uint32 word);
  static uint32 TrimAddress(uint32 address);

  Machine();
  Machine(const InstructionDB* instruction_db, const LogicDB* logic_db);
  ~Machine();

  void Reset();  // clear CPU state and memory, does not affect devices
  ExecuteResult::ResultId Execute();  // execute single instruction
  void Interrupt();

  void ReadMemory(uint32 address, int read_size, uint8* buffer) const;
  uint8 ReadMemoryByte(uint32 address) const;
  uint32 ReadMemoryWord(uint32 address) const;
  void ReadMemoryFloat(uint32 address, uint8* result) const;
  void WriteMemory(uint32 address, int write_size, const uint8* buffer);
  void WriteMemoryByte(uint32 address, uint8 value);
  void WriteMemoryWord(uint32 address, uint32 value);
  void WriteMemoryFloat(uint32 address, const uint8* value);

  Device* GetDevice(uint8 device_id);
  void SetDevice(uint8 device_id, Device* device);  // take ownership of device
  Device* ReleaseDevice(uint8 device_id);  // release ownership of device

  const CpuState& cpu_state() const;
  CpuState* mutable_cpu_state();
  const uint8* memory() const;

 private:
  // for FS34 instructions, returns false if invalid addressing
  bool CalculateTargetAddress(const InstructionInstance& instance);

  const InstructionDB* instruction_db_;
  const LogicDB* logic_db_;

  CpuState cpu_state_;
  std::unique_ptr<uint8[]> memory_;
  std::unique_ptr<Device> devices_[1 << 8];
};

}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_MACHINE_H
