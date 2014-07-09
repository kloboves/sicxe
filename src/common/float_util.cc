#include "common/float_util.h"

namespace sicxe {

namespace {

bool SystemIsBigEndian() {
  union {
    int32 a;
    char b[4];
  } test;
  test.a = 1;
  return test.b[3] == 1;
}

}  // namespace

double FloatUtil::DecodeFloatData(const uint8* data) {
  union {
    double val_double;
    uint8 val_data[8];
  } value_union;
  if (SystemIsBigEndian()) {
    for (int i = 0; i < 6; i++) {
      value_union.val_data[i] = data[i];
    }
    value_union.val_data[6] = 0;
    value_union.val_data[7] = 0;
  } else {
    value_union.val_data[0] = 0;
    value_union.val_data[1] = 0;
    for (int i = 0; i < 6; i++) {
      value_union.val_data[7 - i] = data[i];
    }
  }
  return value_union.val_double;
}

void FloatUtil::EncodeFloatData(double value, uint8* data) {
  union {
    double val_double;
    uint8 val_data[8];
  } value_union;
  value_union.val_double = value;
  if (SystemIsBigEndian()) {
    for (int i = 0; i < 6; i++) {
      data[i] = value_union.val_data[i];
    }
  } else {
    for (int i = 0; i < 6; i++) {
      data[i] = value_union.val_data[7 - i];
    }
  }
}

}  // namespace sicxe
