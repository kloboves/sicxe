#include "common/command_interface.h"

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory>
#include "common/macros.h"

using std::map;
using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;

namespace sicxe {

const size_t CommandInterface::kReadBufferSize = 4096;

namespace {

volatile bool cancellable_action_active = false;
volatile bool cancellable_action_cancelled = false;

void InterruptSignalHandler(int) {
  if (cancellable_action_active) {
    cancellable_action_cancelled = true;
  } else {
    _exit(0);
  }
}

}  // namespace

CommandInterface::CommandInterface(const std::string& prompt)
  : prompt_(prompt), root_menu_(new MenuNode), keep_running_(false) {
  struct sigaction sa;
  sa.sa_handler = &InterruptSignalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  bool success = (sigaction(SIGINT, &sa, nullptr) == 0);
  assert(success);
  unused(success);
  root_menu_->is_leaf = false;
  using std::placeholders::_1;
  RegisterCommand(vector<string>{"exit"},
                  "Exit the application.",
                  vector<pair<string, bool> >{},
                  std::bind(&CommandInterface::ExitCommandHandler, this, _1));
  RegisterCommand(vector<string>{"quit"},
                  "Exit the application.",
                  vector<pair<string, bool> >{},
                  std::bind(&CommandInterface::ExitCommandHandler, this, _1));
}

CommandInterface::~CommandInterface() {}

void CommandInterface::StartCancellableAction() {
  cancellable_action_active = true;
  cancellable_action_cancelled = false;
}

void CommandInterface::EndCancellableAction() {
  cancellable_action_active = false;
}

bool CommandInterface::ActionIsCancelled() {
  return cancellable_action_cancelled;
}

namespace {

CommandInterface::MenuNode* MenuMapFindOrNull(
    map<string, unique_ptr<CommandInterface::MenuNode> >* menu_map,
    const string& menu_name) {
  auto iterator = menu_map->find(menu_name);
  if (iterator == menu_map->end()) {
    return nullptr;
  }
  return iterator->second.get();
}

}  // namespace

void CommandInterface::RegisterCommand(
    const vector<string>& command_path,
    const string& help_description,
    const vector<pair<string, bool> >& command_params,
    CommandCallbackFunc callback) {
  MenuNode* node = root_menu_.get();
  for (size_t i = 0; i < command_path.size(); i++) {
    MenuNode* next_node = MenuMapFindOrNull(&node->submenus, command_path[i]);
    if (i == command_path.size() - 1) {
      assert(next_node == nullptr);
    }
    if (next_node == nullptr) {
      next_node = new MenuNode;
      bool success = node->submenus.insert(
          make_pair(command_path[i], unique_ptr<MenuNode>(next_node))).second;
      assert(success);
      success = node->submenus_ptree.Insert(command_path[i]);
      assert(success);
      next_node->is_leaf = false;
    }
    assert(!next_node->is_leaf);
    node = next_node;
  }

  node->is_leaf = true;
  node->help_description = help_description;
  node->callback = callback;
  for (const auto& params_pair : command_params) {
    bool success = node->parameters.insert(params_pair).second;
    assert(success);
    success = node->parameters_ptree.Insert(params_pair.first);
    assert(success);
  }
}

void CommandInterface::Run() {
  unique_ptr<char[]> read_buffer(new char[kReadBufferSize]);
  int read_length = 0;
  keep_running_ = true;
  while (keep_running_) {
    printf("%s> ", prompt_.c_str());
    if (fgets(read_buffer.get(), kReadBufferSize, stdin) == nullptr) {
      printf("\n");
      break;
    }
    read_length = strlen(read_buffer.get());
    if (read_length > 0 && read_buffer[read_length - 1] == '\n') {
      read_length--;
    }

    vector<string> tokens;
    if (!TokenizeInput(read_buffer.get(), read_length, &tokens)) {
      continue;
    }

    if (tokens.empty()) {  // empty input should do nothing
      continue;
    }

    vector<string> command_path;
    ParsedArgumentMap argument_map;
    if (!ParseInput(tokens, &command_path, &argument_map)) {
      continue;
    }

    if (command_path[command_path.size() - 1] == "?") {
      command_path.pop_back();
      DisplayHelpForMenu(command_path);
    } else {
      ExecuteCommand(command_path, argument_map);
    }
  }
}

void CommandInterface::ExitCommandHandler(const ParsedArgumentMap&) {
  keep_running_ = false;
}

namespace {

void PrintParametersList(const map<string, bool>& parameters) {
  if (!parameters.empty()) {
    bool first_parameter = true;
    for (const auto& parameter_pair : parameters) {
      if (parameter_pair.second) {
        if (!first_parameter) {
          printf(" ");
        }
        printf("%s=", parameter_pair.first.c_str());
        first_parameter = false;
      }
    }
    for (const auto& parameter_pair : parameters) {
      if (!parameter_pair.second) {
        if (!first_parameter) {
          printf(" ");
        }
        printf("[%s=]", parameter_pair.first.c_str());
        first_parameter = false;
      }
    }
  }
}

}  // namespace

CommandInterface::MenuNode* CommandInterface::FindMenuNode(
    const vector<string>& command_path,
    vector<string>* matched_command_path) {
  MenuNode* node = root_menu_.get();
  for (size_t i = 0; i < command_path.size(); i++) {
    vector<string> match_result;
    node->submenus_ptree.Find(command_path[i], &match_result);
    if (match_result.empty()) {
      printf("Error: No such menu '%s'!\n", command_path[i].c_str());
      return nullptr;
    }
    if (match_result.size() > 1) {
      printf("Error: Menu name '%s' is ambiguous (",
             command_path[i].c_str());
      for (size_t j = 0; j < match_result.size(); j++) {
        if (j > 0) {
          printf(", ");
        }
        printf("'%s'", match_result[j].c_str());
      }
      printf(")\n");
      return nullptr;
    }
    MenuNode* next_node = MenuMapFindOrNull(&node->submenus, match_result[0]);
    assert(next_node != nullptr);
    if (i < command_path.size() - 1 && next_node->is_leaf) {
      printf("Error: No sub-menus in '%s'!\n", command_path[i].c_str());
      return nullptr;
    }
    matched_command_path->push_back(match_result[0]);
    node = next_node;
  }
  return node;
}

bool CommandInterface::DisplayHelpForMenu(const vector<string>& command_path) {
  vector<string> matched_path;
  MenuNode* node = FindMenuNode(command_path, &matched_path);
  if (node == nullptr) {
    return false;
  }

  if (!node->is_leaf) {
    printf("Menu types: M - submenu, C - command\n");
    printf(" %-8s %-12s %s\n", "TYPE", "NAME", "PARAMS");
    for (const auto& submenu_pair : node->submenus) {
      if (submenu_pair.second->is_leaf) {
        printf(" %-8s %-12s ", "C", submenu_pair.first.c_str());
        PrintParametersList(submenu_pair.second->parameters);
        printf("\n");
      } else {
        printf(" %-8s %-12s\n", "M", submenu_pair.first.c_str());
      }
    }
  } else {
    printf("%s\n", node->help_description.c_str());
    if (!node->parameters.empty()) {
      printf("Parameters: ");
      PrintParametersList(node->parameters);
      printf("\n");
    }
  }

  return true;
}

bool CommandInterface::ExecuteCommand(const vector<string>& command_path,
                                      const ParsedArgumentMap& arguments) {
  vector<string> matched_path;
  MenuNode* node = FindMenuNode(command_path, &matched_path);
  if (node == nullptr) {
    return false;
  }

  if (!node->is_leaf) {
    printf("Error: Menu '%s' is not a command!\n", matched_path.back().c_str());
    return false;
  }

  // check arguments
  ParsedArgumentMap arguments_matched;
  for (const auto& argument_pair : arguments) {
    vector<string> match_result;
    node->parameters_ptree.Find(argument_pair.first, &match_result);
    if (match_result.empty()) {
      printf("Error: No such parameter '%s'!\n", argument_pair.first.c_str());
      return false;
    }
    if (match_result.size() > 1) {
      printf("Error: Parameter name '%s' is ambiguous (", argument_pair.first.c_str());
      for (size_t j = 0; j < match_result.size(); j++) {
        if (j > 0) {
          printf(", ");
        }
        printf("'%s'", match_result[j].c_str());
      }
      printf(")\n");
      return false;
    }
    arguments_matched.insert(make_pair(match_result[0], argument_pair.second));
  }

  for (const auto& parameter_pair : node->parameters) {
    if (parameter_pair.second) {
      if (arguments_matched.find(parameter_pair.first) == arguments_matched.end()) {
        printf("Error: Parameter '%s' is required!\n", parameter_pair.first.c_str());
        return false;
      }
    }
  }

  node->callback(arguments_matched);
  return true;
}

bool CommandInterface::TokenizeInput(const char* input, int input_length,
                                     vector<string>* tokens) {
  unique_ptr<char[]> token_buffer(new char[kReadBufferSize]);
  int token_length = 0;
  bool quote_open = false;
  for (int index = 0; index <= input_length; index++) {
    if (index == input_length) {
      if (quote_open) {
        printf("Syntax error: Unmatched opening qoute!\n");
        return false;
      } else {
        if (token_length > 0) {
          tokens->push_back(string(token_buffer.get(), token_length));
          token_length = 0;
        }
      }
    } else {
      if (input[index] == '\"') {
        if (quote_open) {
          if (index + 1 < input_length &&
              input[index + 1] != ' ' && input[index + 1] != '=') {
            printf("Syntax error: Expected space after closing quote!\n");
            return false;
          }
          tokens->push_back(string(token_buffer.get(), token_length));
          quote_open = false;
          token_length = 0;
        } else {
          if (token_length > 0) {
            printf("Syntax error: Unexpected opening quote!\n");
            return false;
          }
          quote_open = true;
          token_length = 0;
        }
      } else if (input[index] == '\\') {
        if (index + 1 >= input_length) {
          printf("Syntax error: Unexpected escape sequence at end of input!\n");
          return false;
        }
        index++;
        if (input[index] == '\"') {
          token_buffer[token_length++] = '\"';
        } else if (input[index] == '\\') {
          token_buffer[token_length++] = '\\';
        } else {
          printf("Syntax error: Unsupported escape sequence '\\%c'!\n",
                 input[index]);
          return false;
        }
      } else {
        if (!quote_open && (input[index] == ' ' || input[index] == '=')) {
          if (token_length > 0) {
            tokens->push_back(string(token_buffer.get(), token_length));
            token_length = 0;
          }
          if (input[index] == '=') {
            tokens->push_back("=");
          }
        } else {
          token_buffer[token_length++] = input[index];
        }
      }
    }
  }
  return true;
}

namespace {

bool NormalizeCommandString(string* command) {
  if (command->size() == 0 || !isalpha(command->at(0))) {
    return false;
  }
  for (size_t i = 0; i < command->size(); i++) {
    if (!isalnum(command->at(i)) && command->at(i) != '_' &&
        command->at(i) != '-') {
      return false;
    }
    if (isalpha(command->at(i))) {
      command->at(i) = static_cast<char>(tolower(command->at(i)));
    }
  }
  return true;
}

bool TryParseWord(const string& str, uint32* result) {
  // TODO: Maybe make checks here a bit more strict (overflow?)
  int32 value = 0;
  if (str.length() > 2 && str[0] == '0' && str[1] == 'x') {  // hex
    const char* buffer = str.c_str() + 2;
    char* end_ptr = nullptr;
    value = static_cast<int32>(strtol(buffer, &end_ptr, 16));
    if (end_ptr == buffer || *end_ptr != '\0') {
      return false;
    }
  } else {  // decimal
    const char* buffer = str.c_str();
    char* end_ptr = nullptr;
    value = static_cast<int32>(strtol(buffer, &end_ptr, 10));
    if (end_ptr == buffer || *end_ptr != '\0') {
      return false;
    }
  }
  value &= 0xFFFFFF;
  *result = static_cast<uint32>(value);
  return true;
}

bool TryParseBool(const string& str, bool* result) {
  string normalized = str;
  for (size_t i = 0; i < normalized.size(); i++) {
    if (!isalpha(normalized[i])) {
      return false;
    }
    normalized[i] = static_cast<char>(tolower(normalized[i]));
  }
  if (normalized == "true") {
    *result = true;
    return true;
  }
  if (normalized == "false") {
    *result = false;
    return true;
  }
  return false;
}

}  // namespace

bool CommandInterface::ParseInput(const vector<string>& tokens,
                                  vector<string>* command_path,
                                  ParsedArgumentMap* argument_map) {
  size_t index = 0;
  bool has_arguments = false;
  for (index = 0; index < tokens.size(); index++) {
    if (tokens[index] == "?") {
      if (index != tokens.size() - 1) {
        printf("Parse error: Unexpected input after '?'\n");
        return false;
      }
      command_path->push_back("?");
      break;
    }
    if (index < tokens.size() - 1 && tokens[index + 1] == "=") {
      has_arguments = true;
      break;
    }
    string command = tokens[index];
    if (!NormalizeCommandString(&command)) {
      printf("Parse error: Menu name '%s' is invalid!\n", tokens[index].c_str());
      return false;
    }
    command_path->push_back(command);
  }

  if (command_path->empty()) {
    printf("Parse error: Expected at least one command!\n");
    return false;
  }

  if (has_arguments) {
    // check that there is no more "?"
    for (size_t i = index; i < tokens.size(); i++) {
      if (tokens[i] == "?") {
        printf("Parse error: Unexpected '?' after beginning of argument list!\n");
        return false;
      }
    }

    for (; index < tokens.size() - 2; index += 3) {
      if (tokens[index] == "=") {
        printf("Parse error: Unexpected '=' - expected argument name!\n");
        return false;
      }
      if (tokens[index + 1] != "=") {
        printf("Parse error: Expected '=' instead of '%s'!\n",
               tokens[index + 1].c_str());
        return false;
      }
      if (tokens[index + 2] == "=") {
        printf("Parse error: Unexpected '=' - expected argument value!\n");
        return false;
      }
      string argument_name = tokens[index];
      if (!NormalizeCommandString(&argument_name)) {
        printf("Parse error: Argument name '%s' is invalid!\n", tokens[index].c_str());
        return false;
      }

      auto insert_result = argument_map->insert(
          pair<string, ParsedArgument>(argument_name, ParsedArgument()));
      if (!insert_result.second) {
        printf("Parse error: Duplicate argument name '%s'!\n", argument_name.c_str());
        return false;
      }

      ParsedArgument* parsed_argument = &insert_result.first->second;
      parsed_argument->value_str = tokens[index + 2];

      parsed_argument->is_word = TryParseWord(parsed_argument->value_str,
                                              &parsed_argument->value_word);

      parsed_argument->is_bool = TryParseBool(parsed_argument->value_str,
                                              &parsed_argument->value_bool);
    }

    if (index != tokens.size()) {
      printf("Parse error: Unexpected end of argument list!\n");
      return false;
    }
  }

  return true;
}

}  // namespace sicxe
