#include "common/instruction_db.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <map>
#include "common/error_db.h"
#include "common/instruction.h"
#include "common/macros.h"
#include "common/text_file.h"

using std::make_pair;
using std::map;
using std::pair;
using std::string;
using std::vector;

namespace sicxe {

typedef map<string, Format::FormatId> FormatNameMap;
typedef map<string, Syntax::SyntaxId> SyntaxNameMap;

InstructionDB::InstructionDB() {
  memset(unavailable_mask_, 0x00, (1 << 6) * sizeof(*unavailable_mask_));
}

InstructionDB::~InstructionDB() {}

void InstructionDB::Clear() {
  mnemonic_map_.clear();
  for (size_t i = 0; i < (1 << 8); i++) {
    instructions_[i].reset();
  }
  memset(unavailable_mask_, 0x00, (1 << 6) * sizeof(*unavailable_mask_));
}

void InstructionDB::Register(Instruction* instruction) {
  assert(IsOpcodeAvailable(instruction->opcode(), instruction->format()));
  bool success = mnemonic_map_.insert(
        pair<string, Instruction*>(instruction->mnemonic(), instruction)).second;
  assert(success);
  unused(success);
  instructions_[instruction->opcode()].reset(instruction);

  if (instruction->opcode() != Format::FS34) {
    // mark 6-bit opcode unavailable
    unavailable_mask_[instruction->opcode() >> 2] = true;
  }
}

bool InstructionDB::IsOpcodeAvailable(uint8 opcode, Format::FormatId format) const {
  if (format == Format::FS34) {
    // check if the opcode is a valid 6-bit opcode
    if ((opcode & 0x3) != 0x0) {
      return false;
    }
    // check for conflict with 8-bit opcodes
    if (unavailable_mask_[opcode >> 2]) {
      return false;
    }
  } else {
    // check for conflict with 6-bit opcodes
    uint8 short_opcode = opcode & 0xFC;
    if (instructions_[short_opcode] != nullptr &&
        instructions_[short_opcode]->format() == Format::FS34) {
      return false;
    }
  }
  return instructions_[opcode] == nullptr;
}

const Instruction* InstructionDB::FindOpcode(uint8 opcode) const {
  return instructions_[opcode].get();
}

const Instruction* InstructionDB::FindMnemonic(const std::string& mnemonic) const {
  auto iterator = mnemonic_map_.find(mnemonic);
  if (iterator == mnemonic_map_.end()) {
    return nullptr;
  }
  return iterator->second;
}

namespace {

FormatNameMap* CreateFormatNameMap() {
  FormatNameMap* m = new FormatNameMap;
  m->insert(make_pair("F1",   Format::F1));
  m->insert(make_pair("F2",   Format::F2));
  m->insert(make_pair("FS34", Format::FS34));
  return m;
}

SyntaxNameMap* CreateSyntaxNameMapF1() {
  SyntaxNameMap* m = new SyntaxNameMap;
  m->insert(make_pair("NONE",     Syntax::F1_NONE));
  return m;
}

SyntaxNameMap* CreateSyntaxNameMapF2() {
  SyntaxNameMap* m = new SyntaxNameMap;
  m->insert(make_pair("REG_REG",  Syntax::F2_REG_REG));
  m->insert(make_pair("REG_N",    Syntax::F2_REG_N));
  m->insert(make_pair("REG",      Syntax::F2_REG));
  return m;
}

SyntaxNameMap* CreateSyntaxNameMapFS34() {
  SyntaxNameMap* m = new SyntaxNameMap;
  m->insert(make_pair("LOAD_W",    Syntax::FS34_LOAD_W));
  m->insert(make_pair("LOAD_B",    Syntax::FS34_LOAD_B));
  m->insert(make_pair("LOAD_F",    Syntax::FS34_LOAD_F));
  m->insert(make_pair("STORE_W",   Syntax::FS34_STORE_W));
  m->insert(make_pair("STORE_BX",   Syntax::FS34_STORE_B));
  m->insert(make_pair("STORE_F",   Syntax::FS34_STORE_F));
  m->insert(make_pair("NONE",     Syntax::FS34_NONE));
  return m;
}

}  // namespace

bool InstructionDB::LoadFromFile(const TextFile& file, InstructionDB* instruction_db,
                                 ErrorDB* error_db) {
  static const FormatNameMap* format_name_map = CreateFormatNameMap();
  static const SyntaxNameMap* syntax_name_map_f1 = CreateSyntaxNameMapF1();
  static const SyntaxNameMap* syntax_name_map_f2 = CreateSyntaxNameMapF2();
  static const SyntaxNameMap* syntax_name_map_fs34 = CreateSyntaxNameMapFS34();
  error_db->SetCurrentFile(&file);
  for (size_t row = 0; row < file.lines().size(); row++) {
    const string& line = file.lines()[row];
    // split line by whitespace
    vector<pair<string, TextFile::Position> > tokens;
    string current_token;
    for (size_t column = 0; column <= line.size(); column++) {
      if (column == line.size() || line[column] == ' ' || line[column] == '\t') {
        if (!current_token.empty()) {
          tokens.push_back(make_pair(current_token, TextFile::Position(row,
              column - current_token.size(), current_token.size())));
          current_token.clear();
        }
      } else {
        if (!(isalnum(line[column]) || line[column] == '_')) {
          error_db->AddError(ErrorDB::ERROR, "invalid character",
                             TextFile::Position(row, column, 1));
          return false;
        }
        current_token.push_back(toupper(line[column]));
      }
    }

    // check that line is not empty, not too long and not too short
    if (tokens.empty()) {
      continue;
    }
    if (tokens.size() >= 5) {
      error_db->AddError(ErrorDB::ERROR, "unexpected input at end of line",
                         tokens[4].second, tokens.back().second);
      return false;
    }
    if (tokens.size() < 4) {
      error_db->AddError(ErrorDB::ERROR, "unexpected end of line",
          TextFile::Position(tokens.back().second.row,
            tokens.back().second.column + tokens.back().second.size - 1, 1));
      return false;
    }

    // parse and validate opcode
    const string& opcode_str = tokens[0].first;
    int64 opcode_val = 0;
    const char* opcode_cstr = opcode_str.c_str();
    char* opcode_endptr = nullptr;
    if (opcode_str.size() > 2 && opcode_str[0] == '0' && opcode_str[1] == 'X') {
      opcode_cstr += 2;
      opcode_val = strtoll(opcode_cstr, &opcode_endptr, 16);
    } else {
      opcode_val = strtoll(opcode_cstr, &opcode_endptr, 10);
    }
    if (opcode_endptr == opcode_cstr || *opcode_endptr != '\0') {
      error_db->AddError(ErrorDB::ERROR, "opcode must me a number", tokens[0].second);
      return false;
    }
    if (opcode_val < 0 || opcode_val > 0xff) {
      error_db->AddError(ErrorDB::ERROR, "opcode must be between 0x0 and 0xff",
                         tokens[0].second);
      return false;
    }
    uint8 opcode = static_cast<uint8>(opcode_val);

    // parse and validate mnemonic
    const string& mnemonic = tokens[1].first;
    for (size_t i = 0; i < mnemonic.size(); i++) {
      bool valid = (i == 0) ? isalpha(mnemonic[i]) : isalnum(mnemonic[i]);
      if (!valid) {
        error_db->AddError(ErrorDB::ERROR, "invalid mnemonic", tokens[1].second);
        return false;
      }
    }
    if (instruction_db->FindMnemonic(mnemonic) != nullptr) {
      error_db->AddError(ErrorDB::ERROR, "duplicate mnemonic", tokens[1].second);
      return false;
    }

    // parse and validate format
    Format::FormatId format_id = Format::F1;
    auto format_map_it = format_name_map->find(tokens[2].first);
    if (format_map_it == format_name_map->end()) {
      error_db->AddError(ErrorDB::ERROR, "invalid instruction format", tokens[2].second);
      return false;
    }
    format_id = format_map_it->second;

    // parse and validate syntax type
    Syntax::SyntaxId syntax_id = Syntax::F1_NONE;
    const SyntaxNameMap* syntax_name_map = nullptr;
    switch (format_id) {
      case Format::F1:
        syntax_name_map = syntax_name_map_f1;
        break;
      case Format::F2:
        syntax_name_map = syntax_name_map_f2;
        break;
      case Format::FS34:
        syntax_name_map = syntax_name_map_fs34;
        break;
      default:
        assert(false);
        break;
    }
    auto syntax_map_it = syntax_name_map->find(tokens[3].first);
    if (syntax_map_it == syntax_name_map->end()) {
      error_db->AddError(ErrorDB::ERROR, "invalid instruction syntax type",
                         tokens[3].second);
      return false;
    }
    syntax_id = syntax_map_it->second;

    // check for opcode conflict
    if (!instruction_db->IsOpcodeAvailable(opcode, format_id)) {
      error_db->AddError(ErrorDB::ERROR, "opcode conflicts with other instructions",
                         tokens[0].second);
      return false;
    }

    instruction_db->Register(new Instruction(opcode, mnemonic, format_id, syntax_id));
  }
  return true;
}

}  // namespace sicxe
