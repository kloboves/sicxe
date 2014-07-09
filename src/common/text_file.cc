#include <stdio.h>
#include <memory>
#include "common/text_file.h"

using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {

const size_t TextFile::kReadBufferSize = 4096;

TextFile::Position::Position() : row(0), column(0), size(0) {}

TextFile::Position::Position(int the_row, int the_column, int the_size)
  : row(the_row), column(the_column), size(the_size) {}

TextFile::TextFile() {}
TextFile::~TextFile() {}

bool TextFile::Open(const std::string& the_file_name) {
  FILE* fp = fopen(the_file_name.c_str(), "r");
  if (fp == nullptr) {
    return false;
  }
  file_name_ = the_file_name;
  lines_.clear();
  string current_line;
  unique_ptr<char[]> read_buffer(new char[kReadBufferSize]);
  size_t read_length = 0;
  while ((read_length = fread(read_buffer.get(), 1, kReadBufferSize, fp)) > 0) {
    for (size_t i = 0; i < read_length; i++) {
      if (read_buffer[i] == '\n') {
        lines_.emplace_back(std::move(current_line));
        current_line = string();
      } else if (read_buffer[i] != '\r') {
        current_line.push_back(read_buffer[i]);
      }
    }
  }
  if (!current_line.empty() || lines_.empty()) {
    lines_.emplace_back(std::move(current_line));
  }
  fclose(fp);
  return true;
}

bool TextFile::Save() const {
  return Save(file_name_);
}

bool TextFile::Save(const std::string& destination) const {
  FILE* fp = fopen(destination.c_str(), "w");
  if (fp == nullptr) {
    return false;
  }
  bool success = true;
  for (const auto& line : lines_) {
    if (fprintf(fp, "%s\n", line.c_str()) < 0) {
      success = false;
      break;
    }
  }
  fclose(fp);
  return success;
}

const string& TextFile::file_name() const {
  return file_name_;
}

void TextFile::set_file_name(const string& new_file_name) {
  file_name_ = new_file_name;
}

const vector<string>& TextFile::lines() const {
  return lines_;
}

vector<string>* TextFile::mutable_lines() {
  return &lines_;
}

}  // namespace sicxe
