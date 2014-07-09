#ifndef MACHINE_FILE_DEVICE_H
#define MACHINE_FILE_DEVICE_H

#include <stdio.h>
#include <string>
#include "common/types.h"
#include "machine/device.h"

namespace sicxe {
namespace machine {

class FileDevice : public Device {
 public:
  explicit FileDevice(const std::string& file_name);
  virtual ~FileDevice();

  virtual bool Test();
  virtual bool Read(uint8* result);
  virtual bool Write(uint8 value);

 private:
  std::string file_name_;
  FILE* fp_;
  bool mode_write_;
};

}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_FILE_DEVICE_H
