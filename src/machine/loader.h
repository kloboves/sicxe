#ifndef MACHINE_LOADER_H
#define MACHINE_LOADER_H

#include "common/macros.h"

namespace sicxe {

class ObjectFile;

namespace machine {

class Machine;

class MachineLoader {
 public:
  DISALLOW_INSTANTIATE(MachineLoader);

  static bool LoadObjectFile(const ObjectFile& object_file, Machine* machine);
};

}  // namespace machine
}  // namespace sicxe

#endif  // MACHINE_LOADER_H
