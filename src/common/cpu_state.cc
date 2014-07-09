#include "common/cpu_state.h"

#include <string.h>
#include <map>
#include <utility>

using std::make_pair;
using std::map;
using std::string;

namespace sicxe {

namespace {

typedef map<string, CpuState::RegisterId> RegisterNameMap;

RegisterNameMap* CreateRegisterNameMap() {
  RegisterNameMap* instance = new RegisterNameMap;

  instance->insert(make_pair("A", CpuState::REG_A));
  instance->insert(make_pair("X", CpuState::REG_X));
  instance->insert(make_pair("L", CpuState::REG_L));
  instance->insert(make_pair("B", CpuState::REG_B));
  instance->insert(make_pair("S", CpuState::REG_S));
  instance->insert(make_pair("T", CpuState::REG_T));

  return instance;
}

const RegisterNameMap* GetRegisterNameMapInstance() {
  static const RegisterNameMap* instance = CreateRegisterNameMap();
  return instance;
}

}  // namespace

bool CpuState::RegisterNameToId(const string& name, RegisterId* id) {
  const RegisterNameMap* name_map = GetRegisterNameMapInstance();
  auto it = name_map->find(name);
  if (it == name_map->end()) {
    return false;
  }
  *id = it->second;
  return true;
}

bool CpuState::RegisterIdToName(RegisterId id, string* name) {
  switch (id) {
    case REG_A:
      *name = "A";
      break;
    case REG_X:
      *name = "X";
      break;
    case REG_L:
      *name = "L";
      break;
    case REG_B:
      *name = "B";
      break;
    case REG_S:
      *name = "S";
      break;
    case REG_T:
      *name = "T";
      break;
    default:
      return false;
  }
  return true;
}

CpuState::CpuState()
    : program_counter(0), condition_code(LESS), interrupt_enabled(false),
      interrupt_link(0), interrupt_condition_code(LESS), target_address(0),
      interrupt_enable_next(false) {
  for (int i = 0 ; i < NUM_REGISTERS; i++) {
    registers[i] = 0;
  }
  memset(float_register, 0, 6);
}

CpuState::~CpuState() {}

}  // namespace sicxe
