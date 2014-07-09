#include "assembler/directive.h"

#include <map>
#include <utility>

using std::make_pair;
using std::map;
using std::string;

namespace sicxe {
namespace assembler {

namespace {

typedef map<string, Directive::DirectiveId> DirectiveNameIdMap;

DirectiveNameIdMap* CreateNameIdMapInstance() {
  DirectiveNameIdMap* instance = new DirectiveNameIdMap;

  instance->insert(make_pair("START",  Directive::START));
  instance->insert(make_pair("END",    Directive::END));
  instance->insert(make_pair("ORG",    Directive::ORG));
  instance->insert(make_pair("EQU",    Directive::EQU));
  instance->insert(make_pair("USE",    Directive::USE));
  instance->insert(make_pair("LTORG",  Directive::LTORG));
  instance->insert(make_pair("BASE",   Directive::BASE));
  instance->insert(make_pair("NOBASE", Directive::NOBASE));
  instance->insert(make_pair("EXTDEF", Directive::EXTDEF));
  instance->insert(make_pair("EXTREF", Directive::EXTREF));
  instance->insert(make_pair("BYTE",   Directive::BYTE));
  instance->insert(make_pair("WORD",   Directive::WORD));
  instance->insert(make_pair("RESB",   Directive::RESB));
  instance->insert(make_pair("RESW",   Directive::RESW));

  return instance;
}

const DirectiveNameIdMap* GetNameIdMapInstance() {
  static DirectiveNameIdMap* instance = CreateNameIdMapInstance();
  return instance;
}

}  // namespace

bool Directive::NameToId(const string& name, Directive::DirectiveId* id) {
  const DirectiveNameIdMap* name_id_map = GetNameIdMapInstance();
  auto it = name_id_map->find(name);
  if (it == name_id_map->end()) {
    return false;
  }
  *id = it->second;
  return true;
}

}  // namespace assembler
}  // namespace sicxe
