#include "common/error_formatter.h"

#include <stdio.h>
#include <unistd.h>
#include <algorithm>

using std::string;

namespace sicxe {

ErrorFormatter::ErrorFormatter() : colors_enabled_(true) {}
ErrorFormatter::~ErrorFormatter() {}

void ErrorFormatter::FileUnderline(const TextFile* file,
                                   const TextFile::Position& pos) const {
  assert(pos.row < static_cast<int>(file->lines().size()));
  const string& line = file->lines()[pos.row];
  assert(pos.size > 0);
  assert((pos.column + pos.size - 1) < static_cast<int>(line.size()));
  fprintf(stderr, "%s\n", line.c_str());
  for (int i = 0; i < pos.column; i++) {
    fprintf(stderr, " ");
  }
  if (StdErrIsTTY()) {
    fprintf(stderr, "\x1b[1;32m");
  }
  for (int i = 0; i < pos.size; i++) {
    fprintf(stderr, "^");
  }
  if (StdErrIsTTY()) {
    fprintf(stderr, "\x1b[0m");
  }
  fprintf(stderr, "\n");
}

void ErrorFormatter::PrintError(const ErrorDB::Entry& entry) const {
  if (entry.file == nullptr) {
    fprintf(stderr, "%s: ", application_name_.c_str());
  } else {
    if (entry.has_position) {
      fprintf(stderr, "%s:%d:%d: ", entry.file->file_name().c_str(),
              entry.position.row + 1, entry.position.column + 1);
    } else {
      fprintf(stderr, "%s: ", entry.file->file_name().c_str());
    }
  }
  if (colors_enabled_ && StdErrIsTTY()) {
    const char* color_start_sequence = nullptr;
    switch (entry.severity) {
      case ErrorDB::ERROR:
        color_start_sequence = "\x1b[1;31m";
        break;
      case ErrorDB::WARNING:
        color_start_sequence = "\x1b[1;35m";
        break;
      case ErrorDB::INFO:
        color_start_sequence = "\x1b[1m";
        break;
    }
    fprintf(stderr, "%s", color_start_sequence);
  }
  const char* severity_string = nullptr;
  switch (entry.severity) {
    case ErrorDB::ERROR:
      severity_string = "error:";
      break;
    case ErrorDB::WARNING:
      severity_string = "warning:";
      break;
    case ErrorDB::INFO:
      severity_string = "info:";
      break;
  }
  fprintf(stderr, "%s", severity_string);
  if (colors_enabled_ && StdErrIsTTY()) {
    fprintf(stderr, "\x1b[0m");
  }
  fprintf(stderr, " %s\n", entry.message.c_str());
  if (entry.file != nullptr && entry.has_position) {
    FileUnderline(entry.file, entry.position);
  }
}

void ErrorFormatter::PrintErrors(const ErrorDB& error_db) const {
  for (const auto& entry : error_db.entries()) {
    PrintError(*entry);
  }
  if (error_db.entries().size() > 1) {
    int remaining = 0;
    if (error_db.error_count() > 0) {
      remaining++;
    }
    if (error_db.warning_count() > 0) {
      remaining++;
    }
    if (error_db.info_count() > 0) {
      remaining++;
    }
    if (error_db.error_count() > 0) {
      fprintf(stderr, "%d ", error_db.error_count());
      if (error_db.error_count() > 1) {
        fprintf(stderr, "errors");
      } else {
        fprintf(stderr, "error");
      }
      remaining--;
      if (remaining == 1) {
        fprintf(stderr, " and ");
      } else if (remaining == 2) {
        fprintf(stderr, ", ");
      }
    }
    if (error_db.warning_count() > 0) {
      fprintf(stderr, "%d ", error_db.warning_count());
      if (error_db.warning_count() > 1) {
        fprintf(stderr, "warnings");
      } else {
        fprintf(stderr, "warning");
      }
      remaining--;
      if (remaining == 1) {
        fprintf(stderr, " and ");
      }
    }
    if (error_db.info_count() > 0) {
      fprintf(stderr, "%d ", error_db.info_count());
      if (error_db.info_count() > 1) {
        fprintf(stderr, "informational messages");
      } else {
        fprintf(stderr, "informational message");
      }
    }
    fprintf(stderr, " generated.\n");
  }
}

const string& ErrorFormatter::application_name() const {
  return application_name_;
}

void ErrorFormatter::set_application_name(const string& name) {
  application_name_ = name;
}

bool ErrorFormatter::colors_enabled() const {
  return colors_enabled_;
}

void ErrorFormatter::set_colors_enabled(bool enabled) {
  colors_enabled_ = enabled;
}

bool ErrorFormatter::StdErrIsTTY() {
  static bool stderr_is_tty = isatty(fileno(stderr));
  return stderr_is_tty;
}

}  // namespace sicxe
