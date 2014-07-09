#include <stdio.h>
#include <memory>
#include <string>
#include <vector>
#include "assembler/code.h"
#include "assembler/code_generator.h"
#include "assembler/log_file_writer.h"
#include "assembler/object_file_writer.h"
#include "assembler/parser.h"
#include "assembler/table_builder.h"
#include "common/error_db.h"
#include "common/error_formatter.h"
#include "common/flags_parser.h"
#include "common/instruction_db.h"
#include "common/object_file.h"
#include "common/text_file.h"

using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {
namespace assembler {

const char* kHelpMessage =
"SIC/XE Assembler v1.0.0 by Klemen Kloboves\n"
"\n"
"Usage:    sicasm [-h] [-l log_file] [-o output_file] [-i insn_db] file\n"
"\n"
"Options:\n"
"\n"
"    -o, --output  output_file\n"
"        Write output to output_file.\n"
"\n"
"    -l, --log  log_file\n"
"        Generate log file and write it to log_file.\n"
"\n"
"    -i, --instruction-db  insn_db\n"
"        Read instruction database from insn_db.\n"
"\n"
"    -h, --help\n"
"        Display help.\n"
"\n"
"    --case-sensitive\n"
"        Mnemonics are case sensitive.\n"
"\n"
"    --no-brackets\n"
"        Disable bracket syntax extension.\n"
"\n"
;

class AssemblerDriver {
 public:
  DISALLOW_COPY_AND_MOVE(AssemblerDriver);

  AssemblerDriver() {
    error_formatter_.set_application_name("sicasm");
    flag_instruction_db_ = flags_parser_.AddFlagString("i", "instruction-db");
    flag_output_file_ = flags_parser_.AddFlagString("o", "output");
    flag_log_file_ = flags_parser_.AddFlagString("l", "log");
    flag_help_ = flags_parser_.AddFlagBool("h", "help");
    flag_case_sensitive_ = flags_parser_.AddFlagBool("", "case-sensitive");
    flag_no_brackets_ = flags_parser_.AddFlagBool("", "no-brackets");
  }

  int Main(int argc, char* argv[]) {
    bool success = RealMain(argc, argv);
    error_formatter_.PrintErrors(error_db_);
    return success ? 0 : 1;
  }

 private:
  bool RealMain(int argc, char* argv[]) {
    if (!flags_parser_.ParseFlags(argc, argv, &error_db_)) {
      return false;
    }

    if (argc == 1 || flag_help_->value_bool) {
      PrintHelp();
      return true;
    }

    if (flags_parser_.args().empty()) {
      error_db_.AddError(ErrorDB::ERROR, "no input file", nullptr);
      return false;
    } else if (flags_parser_.args().size() > 1) {
      error_db_.AddError(ErrorDB::ERROR, "expected one input file", nullptr);
      return false;
    }

    const InstructionDB* instruction_db = nullptr;
    if (flag_instruction_db_->is_set) {
      if (!OpenFile(flag_instruction_db_->value_string, &custom_db_file_)) {
        return false;
      }
      custom_db_.reset(new InstructionDB);
      if (!InstructionDB::LoadFromFile(custom_db_file_, custom_db_.get(), &error_db_)) {
        return false;
      }
      instruction_db = custom_db_.get();
    } else {
      instruction_db = InstructionDB::Default();
    }

    if (!OpenFile(flags_parser_.args().front(), &input_file_)) {
      return false;
    }

    Code code;

    Parser::Config parser_config;
    parser_config.instruction_db = instruction_db;
    parser_config.case_sensitive = flag_case_sensitive_->value_bool;
    parser_config.allow_brackets = !flag_no_brackets_->value_bool;
    Parser parser(&parser_config);
    if (!parser.ParseFile(input_file_, &code, &error_db_)) {
      return false;
    }

    TableBuilder table_builder;
    if (!table_builder.BuildTables(&code, &error_db_)) {
      return false;
    }

    bool log_enabled = flag_log_file_->is_set;
    ObjectFileWriter object_writer(&output_file_);
    LogFileWriter log_writer(instruction_db, &log_file_);
    CodeGenerator::OutputWriterVector writers;
    writers.push_back(&object_writer);
    if (log_enabled) {
      writers.push_back(&log_writer);
    }

    CodeGenerator code_generator;
    if (!code_generator.GenerateCode(code, &writers, &error_db_)) {
      return false;
    }

    bool success = true;
    string output_file_name;
    if (flag_output_file_->is_set) {
      output_file_name = flag_output_file_->value_string;
    } else {
      output_file_name = flags_parser_.args().front();
      size_t dot = output_file_name.find_last_of('.');
      if (dot != string::npos) {
        string extension = output_file_name.substr(dot + 1, string::npos);
        if (extension == "asm") {
          output_file_name.resize(dot);
        }
      }
      output_file_name += ".obj";
    }

    if (!output_file_.SaveFile(output_file_name.c_str())) {
      FileWriteError(output_file_name);
      success = false;
    }
    if (log_enabled) {
      if (!log_file_.Save(flag_log_file_->value_string)) {
        FileWriteError(flag_log_file_->value_string);
        success = false;
      }
    }

    return success;
  }

  bool OpenFile(const string& file_name, TextFile* file) {
    if (!file->Open(file_name)) {
      string message = "cannot open file '" + file_name + "'";
      error_db_.AddError(ErrorDB::ERROR, message.c_str(), nullptr);
      return false;
    }
    return true;
  }

  void FileWriteError(const string& file_name) {
    string message = "cannot write file '" + file_name + "'";
    error_db_.AddError(ErrorDB::ERROR, message.c_str(), nullptr);
  }

  void PrintHelp() {
    printf("%s\n", kHelpMessage);
  }

  ErrorDB error_db_;
  ErrorFormatter error_formatter_;
  FlagsParser flags_parser_;
  const FlagsParser::Flag* flag_instruction_db_;
  const FlagsParser::Flag* flag_output_file_;
  const FlagsParser::Flag* flag_log_file_;
  const FlagsParser::Flag* flag_help_;
  const FlagsParser::Flag* flag_case_sensitive_;
  const FlagsParser::Flag* flag_no_brackets_;
  TextFile custom_db_file_;
  TextFile input_file_;
  TextFile log_file_;
  ObjectFile output_file_;
  unique_ptr<InstructionDB> custom_db_;
};

}  // namespace assembler
}  // namespace sicxe

int main(int argc, char* argv[]) {
  return sicxe::assembler::AssemblerDriver().Main(argc, argv);
}
