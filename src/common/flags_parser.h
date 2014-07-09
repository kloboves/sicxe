#ifndef COMMON_FLAGS_PARSER_H
#define COMMON_FLAGS_PARSER_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"

namespace sicxe {

class ErrorDB;

class FlagsParser {
 public:
  DISALLOW_COPY_AND_MOVE(FlagsParser);

  enum FlagTypeId {
    BOOLEAN = 0,
    STRING = 1
  };

  struct Flag {
    Flag();

    FlagTypeId type;
    std::string name_short;
    std::string name_long;
    bool is_set;
    bool value_bool;
    std::string value_string;
  };

  typedef std::vector<std::unique_ptr<Flag> > FlagVector;
  typedef std::vector<std::string> StringVector;

  FlagsParser();
  ~FlagsParser();

  const Flag* AddFlagBool(const std::string& name_short, const std::string& name_long);
  const Flag* AddFlagString(const std::string& name_short, const std::string& name_long);

  bool ParseFlags(int argc, const char* const* argv, ErrorDB* error_db);

  const FlagVector& flags() const;
  const StringVector& args() const;

 private:
  const Flag* AddFlag(FlagTypeId type, const std::string& name_short,
                      const std::string& name_long);

  FlagVector flags_;
  StringVector args_;
  std::map<std::string, Flag*> name_map_short_;
  std::map<std::string, Flag*> name_map_long_;
};

}  // namespace sicxe

#endif  // COMMON_FLAGS_PARSER_H
