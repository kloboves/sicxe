#ifndef FPGA_CONNECTION_H
#define FPGA_CONNECTION_H

#include <string>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {
namespace fpga {

class FpgaConnection {
 public:
  DISALLOW_COPY_AND_MOVE(FpgaConnection);

  static const int kReadTimeoutMs;

  explicit FpgaConnection(const std::string& device_file_name);
  ~FpgaConnection();

  bool Open();
  void Close();
  bool TestConnection();

  bool ControlSignal(uint8 sig);  // trigger a circuit control signal
  bool WriteMemory(uint32 address, int write_size, const uint8* buffer);
  bool ReadMemory(uint32 address, int read_size, uint8* buffer);

  bool WriteMemoryByte(uint32 address, uint8 value);
  bool WriteMemoryWord(uint32 address, uint32 value);
  bool ReadMemoryByte(uint32 address, uint8* result);
  bool ReadMemoryWord(uint32 address, uint32* result);

 private:
  bool SendCommand(uint8 command);
  bool SendCommandNoUnlock(uint8 command);
  bool UnlockProcedure();
  bool SendDataTransferHeader(uint32 address, int size);

  std::string device_file_name_;
  int device_fd_;
  bool unlock_success_;
};

}  // namespace fpga
}  // namespace sicxe

#endif  // FPGA_CONNECTION_H
