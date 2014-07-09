#include "fpga/loader.h"

#include <memory>
#include "common/object_file.h"
#include "fpga/connection.h"

using std::unique_ptr;

namespace sicxe {
namespace fpga {

bool FpgaLoader::LoadObjectFile(const ObjectFile& object_file,
                                FpgaConnection* connection) {
  if (object_file.start_address() != 0 || object_file.entry_point() != 0 ||
      !object_file.import_sections().empty()) {
    return false;
  }
  for (const auto& section : object_file.text_sections()) {
    if (!connection->WriteMemory(section->address, section->size, section->data.get())) {
      return false;
    }
  }
  return true;
}

}  // namespace fpga
}  // namespace sicxe
