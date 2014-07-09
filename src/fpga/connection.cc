#include "fpga/connection.h"

#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using std::string;

namespace sicxe {
namespace fpga {

const int FpgaConnection::kReadTimeoutMs = 100;

static const char* kUnlockKeySend = "SICXE";
static const int kUnlockKeySendSize = 5;
static const char* kUnlockKeyReceive = "ACK";
static const int kUnlockKeyReceiveSize = 3;

FpgaConnection::FpgaConnection(const string& device_file_name)
  : device_file_name_(device_file_name), device_fd_(-1), unlock_success_(false) {}

FpgaConnection::~FpgaConnection() {
  Close();
}

bool FpgaConnection::Open() {
  device_fd_ = open(device_file_name_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (device_fd_ < 0) {
    return false;
  }

  struct termios tio;
  memset(&tio, 0, sizeof(tio));
  tio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
  tio.c_iflag = IGNPAR | IGNBRK;
  tio.c_oflag = 0;
  tio.c_lflag = 0;
  tio.c_cc[VTIME] = 0;
  tio.c_cc[VMIN] = 1;
  cfsetspeed(&tio, B115200);

  if (tcflush(device_fd_, TCIFLUSH) != 0 ||
      tcsetattr(device_fd_, TCSANOW, &tio) != 0) {
    return false;
  }
  return true;
}

void FpgaConnection::Close() {
  if (unlock_success_) {
    SendCommandNoUnlock(0xff);  // lock device
  }
  if (device_fd_ < 0) {
    close(device_fd_);
  }
}

bool FpgaConnection::SendCommandNoUnlock(uint8 command) {
  if (write(device_fd_, &command, 1) != 1) {
    return false;
  }
  struct pollfd pfd;
  pfd.fd = device_fd_;
  pfd.events = POLLIN;
  if (poll(&pfd, 1, kReadTimeoutMs) != 1) {
    return false;
  }
  uint8 read_buffer[1];
  if (read(device_fd_, read_buffer, 1) != 1) {
    return false;
  }
  return read_buffer[0] == 'K';
}

bool FpgaConnection::UnlockProcedure() {
  if (write(device_fd_, kUnlockKeySend, kUnlockKeySendSize) != kUnlockKeySendSize) {
    return false;
  }
  struct pollfd pfd;
  pfd.fd = device_fd_;
  pfd.events = POLLIN;
  uint8 key_buffer[kUnlockKeyReceiveSize];
  for (int i = 0; i < kUnlockKeyReceiveSize; i++) {
    if (poll(&pfd, 1, kReadTimeoutMs) != 1) {
      return false;
    }
    if (read(device_fd_, key_buffer + i, 1) != 1) {
      return false;
    }
  }
  for (int i = 0; i < kUnlockKeyReceiveSize; i++) {
    if (key_buffer[i] != kUnlockKeyReceive[i]) {
      return false;
    }
  }
  return true;
}

bool FpgaConnection::SendCommand(uint8 command) {
  bool success = true;
  if (!SendCommandNoUnlock(command)) {
    success = UnlockProcedure();
    if (success) {
      success = SendCommandNoUnlock(command);
    }
  }
  unlock_success_ = success;
  return success;
}

bool FpgaConnection::TestConnection() {
  if (device_fd_ < 0) {
    return false;
  }
  return SendCommand(0);
}

bool FpgaConnection::ControlSignal(uint8 sig) {
  if (device_fd_ < 0 || sig > 0x3) {
    return false;
  }
  return SendCommand(0x10 | sig);
}

bool FpgaConnection::SendDataTransferHeader(uint32 address, int size) {
  uint8 send_buffer[5];
  for (int i = 0; i < 3; i++) {
    send_buffer[i] = address & 0xff;
    address >>= 8;
  }
  for (int i = 0; i < 2; i++) {
    send_buffer[3 + i] = size & 0xff;
    size >>= 8;
  }
  return write(device_fd_, send_buffer, 5) == 5;
}

bool FpgaConnection::WriteMemory(uint32 address, int write_size, const uint8* buffer) {
  if (write_size > 0xffff) {
    return false;
  }
  if (!SendCommand(0x02) || !SendDataTransferHeader(address, write_size)) {
    return false;
  }
  return write(device_fd_, buffer, write_size) == write_size;
}

bool FpgaConnection::ReadMemory(uint32 address, int read_size, uint8* buffer) {
  if (read_size > 0xffff) {
    return false;
  }
  if (!SendCommand(0x01) || !SendDataTransferHeader(address, read_size)) {
    return false;
  }

  struct pollfd pfd;
  pfd.fd = device_fd_;
  pfd.events = POLLIN;

  int total_read = 0;
  while (total_read < read_size) {
    if (poll(&pfd, 1, kReadTimeoutMs) != 1) {
      return false;
    }
    total_read += read(device_fd_, buffer + total_read, read_size - total_read);
  }
  return true;
}

bool FpgaConnection::WriteMemoryByte(uint32 address, uint8 value) {
  return WriteMemory(address, 1, &value);
}

bool FpgaConnection::WriteMemoryWord(uint32 address, uint32 value) {
  uint8 buffer[3];
  buffer[2] = value & 0xFF;
  value >>= 8;
  buffer[1] = value & 0xFF;
  value >>= 8;
  buffer[0] = value & 0xFF;
  return WriteMemory(address, 3, buffer);
}

bool FpgaConnection::ReadMemoryByte(uint32 address, uint8* result) {
  return ReadMemory(address, 1, result);
}

bool FpgaConnection::ReadMemoryWord(uint32 address, uint32* result) {
  uint8 buffer[3];
  if (!ReadMemory(address, 3, buffer)) {
    return false;
  }
  uint32 word = static_cast<uint32>(buffer[0]);
  word <<= 8;
  word |= static_cast<uint32>(buffer[1]);
  word <<= 8;
  word |= static_cast<uint32>(buffer[2]);
  *result = word;
  return true;
}

}  // namespace fpga
}  // namespace sicxe
