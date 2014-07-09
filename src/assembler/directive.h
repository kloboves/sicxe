#ifndef ASSEMBLER_DIRECTIVE_H
#define ASSEMBLER_DIRECTIVE_H

#include <string>
#include "common/macros.h"

namespace sicxe {
namespace assembler {

class Directive {
 public:
  DISALLOW_INSTANTIATE(Directive);

  enum DirectiveId {
    START = 0,
    END,
    ORG,
    EQU,
    USE,
    LTORG,
    BASE,
    NOBASE,
    EXTDEF,
    EXTREF,
    BYTE,
    WORD,
    RESB,
    RESW,
    INTERNAL_LITERAL,  // special internal directive for marking literals

    NUM_DIRECTIVES  // keep last in enum
  };

  static bool NameToId(const std::string& name, DirectiveId* id);
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_DIRECTIVE_H
