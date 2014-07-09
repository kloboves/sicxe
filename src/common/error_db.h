#ifndef COMMON_ERROR_DB_H
#define COMMON_ERROR_DB_H

#include <memory>
#include <list>
#include <string>
#include "common/macros.h"
#include "common/text_file.h"

namespace sicxe {

class ErrorDB {
 public:
  DISALLOW_COPY_AND_MOVE(ErrorDB);

  enum SeverityId {
    ERROR = 0,
    WARNING,
    INFO
  };

  struct Entry {
    DISALLOW_COPY_AND_MOVE(Entry);

    Entry();

    SeverityId severity;
    std::string message;
    const TextFile* file;
    bool has_position;
    TextFile::Position position;
  };

  typedef std::list<std::unique_ptr<Entry> > EntryList;

  ErrorDB();
  ~ErrorDB();

  // Add a new error. Set |file| = nullptr for general errors not in a text file.
  void AddError(SeverityId severity, const char* message, const TextFile* file);
  void AddError(SeverityId severity, const char* message, const TextFile::Position& pos,
                const TextFile* file);
  void AddError(SeverityId severity, const char* message,
                const TextFile::Position& start, const TextFile::Position& end,
                const TextFile* file);

  // Same as above, but with |file| = |current_file_|.
  void AddError(SeverityId severity, const char* message);
  void AddError(SeverityId severity, const char* message, const TextFile::Position& pos);
  void AddError(SeverityId severity, const char* message,
                const TextFile::Position& start, const TextFile::Position& end);

  void SetCurrentFile(const TextFile* file);

  const EntryList& entries() const;
  int error_count() const;
  int warning_count() const;
  int info_count() const;

 private:
  void IncrementCounter(SeverityId severity);

  const TextFile* current_file_;
  EntryList entries_;
  int error_count_;
  int warning_count_;
  int info_count_;
};

}  // namespace sicxe

#endif  // COMMON_ERROR_DB_H
