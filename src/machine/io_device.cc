#include "machine/io_device.h"

namespace sicxe {
namespace machine {

// InputDevice implementation
InputDevice::InputDevice() {}
InputDevice::~InputDevice() {}

bool InputDevice::Test() {
  return true;
}

bool InputDevice::Read(uint8* result) {
  if (fread(result, 1, 1, stdin) != 1) {
    *result = 0;
  }
  return true;
}

bool InputDevice::Write(uint8) {
  return false;
}

// OutputDevice implementation
OutputDevice::OutputDevice(bool is_stderr) : is_stderr_(is_stderr) {}
OutputDevice::~OutputDevice() {}

bool OutputDevice::Test() {
  return true;
}

bool OutputDevice::Read(uint8*) {
  return false;
}

bool OutputDevice::Write(uint8 value) {
  if (is_stderr_) {
    fwrite(&value, 1, 1, stderr);
    fflush(stderr);
  } else {
    fwrite(&value, 1, 1, stdout);
    fflush(stdout);
  }
  return true;
}

}  // namespace machine
}  // namespace sicxe
