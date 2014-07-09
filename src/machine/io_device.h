#ifndef MACHINE_IO_DEVICE_H
#define MACHINE_IO_DEVICE_H

#include <stdio.h>
#include "common/types.h"
#include "machine/device.h"

namespace sicxe {
namespace machine {

class InputDevice : public Device {
 public:
  InputDevice();
  virtual ~InputDevice();

  virtual bool Test();
  virtual bool Read(uint8* result);
  virtual bool Write(uint8 value);
};

class OutputDevice : public Device {
 public:
  explicit OutputDevice(bool is_stderr);
  virtual ~OutputDevice();

  virtual bool Test();
  virtual bool Read(uint8* result);
  virtual bool Write(uint8 value);

 private:
  bool is_stderr_;
};

}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_IO_DEVICE_H
