#include "machine/file_device.h"

using std::string;

namespace sicxe {
namespace machine {

FileDevice::FileDevice(const std::string& file_name)
  : file_name_(file_name), fp_(nullptr), mode_write_(false) {}

FileDevice::~FileDevice() {
  if (fp_) {
    fclose(fp_);
  }
}

bool FileDevice::Test() {
  // TODO: Maybe change this
  return true;
}

bool FileDevice::Read(uint8* result) {
  if (fp_ == nullptr) {
    fp_ = fopen(file_name_.c_str(), "r");
    if (fp_ == nullptr) {
      return false;
    }
    mode_write_ = false;
  }

  if (mode_write_) {
    return false;
  }
  if (fread(result, 1, 1, fp_) != 1) {
    *result = 0;
  }
  return true;
}

bool FileDevice::Write(uint8 value) {
  if (fp_ == nullptr) {
    fp_ = fopen(file_name_.c_str(), "w");
    if (fp_ == nullptr) {
      return false;
    }
    mode_write_ = true;
  }

  if (!mode_write_) {
    return false;
  }
  fwrite(&value, 1, 1, fp_);
  return true;
}

}  // namespace machine
}  // namespace sicxe
