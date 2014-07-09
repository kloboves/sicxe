#ifndef MACHINE_DEVICE_H
#define MACHINE_DEVICE_H

#include "common/macros.h"
#include "common/types.h"

namespace sicxe {
namespace machine {

class Device {
 public:
  DISALLOW_COPY_AND_MOVE(Device);

  Device();
  virtual ~Device();

  virtual bool Test() = 0;
  virtual bool Read(uint8* result) = 0;
  virtual bool Write(uint8 value) = 0;
};

}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_DEVICE_H
