#include "common/flags_parser.h"

#include <assert.h>
#include "common/error_db.h"

using std::string;
using std::unique_ptr;

namespace sicxe {

FlagsParser::Flag::Flag() : type(BOOLEAN), is_set(false), value_bool(false) {}

FlagsParser::FlagsParser() {}
FlagsParser::~FlagsParser() {}

const FlagsParser::Flag* FlagsParser::AddFlag(FlagTypeId type, const string& name_short,
                                              const string& name_long) {
  unique_ptr<Flag> flag(new Flag);
  flag->type = type;
  flag->name_short = name_short;
  flag->name_long = name_long;
  if (!flag->name_short.empty()) {
    bool success = name_map_short_.insert(make_pair(flag->name_short, flag.get())).second;
    assert(success);
    unused(success);
  }
  if (!flag->name_long.empty()) {
    bool success = name_map_long_.insert(make_pair(flag->name_long, flag.get())).second;
    assert(success);
    unused(success);
  }
  const Flag* rv = flag.get();
  flags_.emplace_back(std::move(flag));
  return rv;
}

const FlagsParser::Flag* FlagsParser::AddFlagBool(const string& name_short,
                                                  const string& name_long) {
  return AddFlag(BOOLEAN, name_short, name_long);
}

const FlagsParser::Flag* FlagsParser::AddFlagString(const string& name_short,
                                                    const string& name_long) {
  return AddFlag(STRING, name_short, name_long);
}

bool FlagsParser::ParseFlags(int argc, const char* const* argv, ErrorDB* error_db) {
  error_db->SetCurrentFile(nullptr);
  bool success = true;
  bool mode = true;
  for (int i = 1; i < argc; i++) {
    if (!mode) {
      args_.push_back(argv[i]);
    } else {
      string arg = argv[i];
      if (arg == "-" || arg == "--") {
        mode = false;
      } else if (arg.size() > 1 && arg[0] == '-') {
        Flag* flag = nullptr;
        if (arg.size() > 2 && arg[1] == '-') {
          string name = arg.substr(2, string::npos);
          auto it = name_map_long_.find(name);
          if (it != name_map_long_.end()) {
            flag = it->second;
          }
        } else {
          string name = arg.substr(1, string::npos);
          auto it = name_map_short_.find(name);
          if (it != name_map_short_.end()) {
            flag = it->second;
          }
        }
        if (flag == nullptr) {
          success = false;
          string message = "illegal option '" + arg + "'";
          error_db->AddError(ErrorDB::ERROR, message.c_str());
          continue;
        }
        if (flag->is_set) {
          success = false;
          string message = "duplicate option '" + arg + "'";
          error_db->AddError(ErrorDB::ERROR, message.c_str());
          continue;
        }
        flag->is_set = true;
        if (flag->type == BOOLEAN) {
          flag->value_bool = true;
        } else {
          i++;
          if (i == argc) {
            string message = "missing argument for option '" + arg + "'";
            error_db->AddError(ErrorDB::ERROR, message.c_str());
            return false;
          }
          flag->value_string = argv[i];
        }
      } else {
        args_.push_back(arg);
      }
    }
  }
  return success;
}

const FlagsParser::FlagVector& FlagsParser::flags() const {
  return flags_;
}

const FlagsParser::StringVector& FlagsParser::args() const {
  return args_;
}

}  // namespace sicxe
