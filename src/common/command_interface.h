#ifndef COMMON_COMMAND_INTERFACE_H
#define COMMON_COMMAND_INTERFACE_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"
#include "common/prefix_tree.h"
#include "common/types.h"

namespace sicxe {

namespace tests { class CommandInterfaceTest; }

class CommandInterface {
 public:
  DISALLOW_COPY_AND_MOVE(CommandInterface);

  struct ParsedArgument {
    std::string value_str;
    bool is_word;
    uint32 value_word;
    bool is_bool;
    bool value_bool;
  };

  typedef std::map<std::string, ParsedArgument> ParsedArgumentMap;
  typedef std::function<void(const ParsedArgumentMap&)> CommandCallbackFunc;

  struct MenuNode {
    bool is_leaf;

    // for non-leaf nodes
    std::map<std::string, std::unique_ptr<MenuNode> > submenus;
    PrefixTree submenus_ptree;

    // for leaf nodes
    std::string help_description;
    CommandCallbackFunc callback;
    std::map<std::string, bool> parameters;
    PrefixTree parameters_ptree;
  };

  static const size_t kReadBufferSize;

  explicit CommandInterface(const std::string& prompt);
  ~CommandInterface();

  // Register a command. Commands "exit" and "quit" are already reserved.
  // |command_params| is a vector of pairs <param name>:<param required>.
  void RegisterCommand(const std::vector<std::string>& command_path,
                       const std::string& help_description,
                       const std::vector<std::pair<std::string, bool> >& command_params,
                       CommandCallbackFunc callback);

  // Start the command interface.
  void Run();

  // For commands that can be cancelled with Ctrl+C
  void StartCancellableAction();
  void EndCancellableAction();
  bool ActionIsCancelled();

 private:
  void ExitCommandHandler(const ParsedArgumentMap& arguments);
  MenuNode* FindMenuNode(const std::vector<std::string>& command_path,
                         std::vector<std::string>* matched_command_path);
  bool DisplayHelpForMenu(const std::vector<std::string>& command_path);
  bool ExecuteCommand(const std::vector<std::string>& command_path,
                      const ParsedArgumentMap& arguments);

  static bool TokenizeInput(const char* input, int input_length,
                            std::vector<std::string>* tokens);
  static bool ParseInput(const std::vector<std::string>& tokens,
                         std::vector<std::string>* command_path,
                         ParsedArgumentMap* argument_map);

  std::string prompt_;
  std::unique_ptr<MenuNode> root_menu_;
  bool keep_running_;

  friend class tests::CommandInterfaceTest;
};

}  // namespace sicxe

#endif  // COMMON_COMMAND_INTERFACE_H
