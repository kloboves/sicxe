#ifndef COMMON_FLOAT_UTIL_H
#define COMMON_FLOAT_UTIL_H

#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class FloatUtil {
 public:
  DISALLOW_INSTANTIATE(FloatUtil);

  static double DecodeFloatData(const uint8* data);
  static void EncodeFloatData(double value, uint8* data);
};

}  // namespace sicxe

#endif  // COMMON_FLOAT_UTIL_H
