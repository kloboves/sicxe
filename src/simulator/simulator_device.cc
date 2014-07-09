#include "simulator/simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string>
#include <memory>
#include "machine/file_device.h"

using sicxe::machine::FileDevice;
using std::string;
using std::unique_ptr;

namespace sicxe {
namespace simulator {

void Simulator::CommandDeviceReset(
    const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& number_arg = arguments.find("number")->second;
  if (!number_arg.is_word) {
    printf("Error: 'number' must be a number!\n");
    return;
  }

  int device_id = number_arg.value_word;
  machine_.SetDevice(device_id, nullptr);
  unique_ptr<char[]> device_name_buffer(new char[PATH_MAX]);
  sprintf(device_name_buffer.get(), "%02X.dev", device_id);
  machine_.SetDevice(device_id, new FileDevice(string(device_name_buffer.get())));
}

}  // namespace simulator
}  // namespace sicxe
