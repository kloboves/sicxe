#include "machine/loader.h"

#include <memory>
#include "machine/machine.h"
#include "common/object_file.h"

using std::unique_ptr;

namespace sicxe {
namespace machine {

bool MachineLoader::LoadObjectFile(const ObjectFile& object_file, Machine* machine) {
  if (!object_file.import_sections().empty()) {
    return false;
  }
  for (const auto& section : object_file.text_sections()) {
    machine->WriteMemory(section->address, section->size, section->data.get());
  }
  machine->mutable_cpu_state()->program_counter =
      Machine::TrimAddress(object_file.entry_point());
  return true;
}

}  // namespace machine
}  // namespace sicxe
