#include <assert.h>
#include "common/error_db.h"

using std::string;
using std::unique_ptr;

namespace sicxe {

ErrorDB::Entry::Entry() : severity(ERROR), file(nullptr), has_position(false) {}

ErrorDB::ErrorDB() : current_file_(nullptr), error_count_(0),
                     warning_count_(0), info_count_(0) {}
ErrorDB::~ErrorDB() {}

void ErrorDB::AddError(SeverityId severity, const char* message, const TextFile* file) {
  unique_ptr<Entry> entry(new Entry);
  entry->severity = severity;
  entry->message = string(message);
  entry->file = file;
  entry->has_position = false;
  entries_.emplace_back(std::move(entry));
  IncrementCounter(severity);
}

void ErrorDB::AddError(SeverityId severity, const char* message,
                       const TextFile::Position& pos, const TextFile* file) {
  assert(file != nullptr);
  unique_ptr<Entry> entry(new Entry);
  entry->severity = severity;
  entry->message = string(message);
  entry->file = file;
  entry->has_position = true;
  entry->position = pos;
  entries_.emplace_back(std::move(entry));
  IncrementCounter(severity);
}

void ErrorDB::AddError(SeverityId severity, const char* message,
                       const TextFile::Position& start, const TextFile::Position& end,
                       const TextFile* file) {
  assert(start.row == end.row);
  assert(start.column <= end.column);
  TextFile::Position pos(start.row, start.column, end.column - start.column + end.size);
  AddError(severity, message, pos, file);
}

void ErrorDB::AddError(SeverityId severity, const char* message) {
  AddError(severity, message, current_file_);
}

void ErrorDB::AddError(SeverityId severity, const char* message,
                       const TextFile::Position& pos) {
  AddError(severity, message, pos, current_file_);
}

void ErrorDB::AddError(SeverityId severity, const char* message,
              const TextFile::Position& start, const TextFile::Position& end) {
  AddError(severity, message, start, end, current_file_);
}

void ErrorDB::SetCurrentFile(const TextFile* file) {
  current_file_ = file;
}

const ErrorDB::EntryList& ErrorDB::entries() const {
  return entries_;
}

int ErrorDB::error_count() const {
  return error_count_;
}

int ErrorDB::warning_count() const {
  return warning_count_;
}

int ErrorDB::info_count() const {
  return info_count_;
}

void ErrorDB::IncrementCounter(SeverityId severity) {
  switch (severity) {
    case ERROR:
      error_count_++;
      break;
    case WARNING:
      warning_count_++;
      break;
    case INFO:
      info_count_++;
      break;
  }
}

}  // namespace sicxe
