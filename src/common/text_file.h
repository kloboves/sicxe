#ifndef COMMON_TEXT_FILE_H
#define COMMON_TEXT_FILE_H

#include <string>
#include <vector>
#include "common/macros.h"

namespace sicxe {

class TextFile {
 public:
  DISALLOW_COPY_AND_MOVE(TextFile);

  struct Position {
    Position();
    Position(int the_row, int the_column, int the_size);

    int row;
    int column;
    int size;
  };

  static const size_t kReadBufferSize;

  TextFile();
  ~TextFile();

  bool Open(const std::string& the_file_name);
  bool Save() const;
  bool Save(const std::string& destination) const;

  const std::string& file_name() const;
  void set_file_name(const std::string& new_file_name);
  const std::vector<std::string>& lines() const;
  std::vector<std::string>* mutable_lines();

 private:
  std::string file_name_;
  std::vector<std::string> lines_;
};

}  // namespace sicxe

#endif  // COMMON_TEXT_FILE_H
