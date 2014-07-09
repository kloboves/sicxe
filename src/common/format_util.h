#ifndef COMMON_FORMAT_UTIL_H
#define COMMON_FORMAT_UTIL_H

#include "common/instruction_instance.h"
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class InstructionDB;
struct InstructionInstance;

class FormatUtil {
 public:
  DISALLOW_INSTANTIATE(FormatUtil);

  static bool Decode(const InstructionDB* instruction_db,
                     const uint8* raw_insn_data,
                     InstructionInstance* instance,
                     int* decoded_length);

  static bool Encode(const InstructionInstance& instance,
                     uint8* raw_insn_data,
                     int* encoded_length);
};

}  // namespace sicxe

#endif  // COMMON_FORMAT_UTIL_H
