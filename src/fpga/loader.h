#ifndef FPGA_LOADER_H
#define FPGA_LOADER_H

#include "common/macros.h"

namespace sicxe {

class ObjectFile;

namespace fpga {

class FpgaConnection;

class FpgaLoader {
 public:
  DISALLOW_INSTANTIATE(FpgaLoader);

  static bool LoadObjectFile(const ObjectFile& object_file, FpgaConnection* connection);
};

}  // namespace fpga
}  // namespace sicxe

#endif  // FPGA_LOADER_H
