#include "simulator/simulator.h"

#include <limits.h>
#include <stdlib.h>
#include <memory>
#include "common/error_db.h"
#include "common/object_file.h"
#include "linker/linker.h"
#include "machine/loader.h"

using sicxe::linker::Linker;
using sicxe::machine::MachineLoader;
using std::unique_ptr;

namespace sicxe {
namespace simulator {

void Simulator::CommandLoad(const CommandInterface::ParsedArgumentMap& arguments) {
  const CommandInterface::ParsedArgument& filename_arg = arguments.find("file")->second;
  ObjectFile object_file;
  ObjectFile::LoadResult rv = object_file.LoadFile(filename_arg.value_str.c_str());
  if (rv == ObjectFile::OPEN_FAILED) {
    printf("Error: Could not open object file!\n");
    return;
  }
  if (rv == ObjectFile::INVALID_FORMAT) {
    printf("Error: Invalid object file!\n");
    return;
  }
  if (rv != ObjectFile::OK) {
    printf("Error: Unknown error occured when opening object file!\n");
    return;
  }

  auto it = arguments.find("address");
  if (it == arguments.end()) {
    // load object file
    machine_.Reset();
    if (!MachineLoader::LoadObjectFile(object_file, &machine_)) {
      printf("Error: Failed to load object file!\n");
      return;
    }
  } else {
    const CommandInterface::ParsedArgument& address_arg = it->second;
    if (!address_arg.is_word) {
      printf("Error: Address must be a number!\n");
      return;
    }
    // relocate object file
    ObjectFile relocated_file;
    ErrorDB error_db;
    Linker::Config config;
    Linker linker(&config);
    if (!linker.LinkFiles(false, address_arg.value_word,
                          Linker::ObjectFileVector{&object_file}, &relocated_file,
                          &error_db)) {
      printf("Error: Failed to relocate object file!\n");
      return;
    }
    // load relocated object file
    machine_.Reset();
    if (!MachineLoader::LoadObjectFile(relocated_file, &machine_)) {
      printf("Error: Failed to load object file!\n");
      return;
    }
  }
}

}  // namespace simulator
}  // namespace sicxe
