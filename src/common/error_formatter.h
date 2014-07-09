#ifndef COMMON_ERROR_FORMATTER_H
#define COMMON_ERROR_FORMATTER_H

#include <string>
#include "common/error_db.h"
#include "common/macros.h"
#include "common/text_file.h"

namespace sicxe {

class ErrorFormatter {
 public:
  DISALLOW_COPY_AND_MOVE(ErrorFormatter);

  ErrorFormatter();
  ~ErrorFormatter();

  void PrintErrors(const ErrorDB& error_db) const;

  const std::string& application_name() const;
  void set_application_name(const std::string& name);
  bool colors_enabled() const;
  void set_colors_enabled(bool enabled);

 private:
  void FileUnderline(const TextFile* file, const TextFile::Position& pos) const;
  void PrintError(const ErrorDB::Entry& entry) const;

  static bool StdErrIsTTY();

  std::string application_name_;
  bool colors_enabled_;
};

}  // namespace sicxe

#endif  // COMMON_ERROR_FORMATTER_H
