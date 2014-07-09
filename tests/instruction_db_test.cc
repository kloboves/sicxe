#include <gtest/gtest.h>
#include <string>
#include "common/error_db.h"
#include "common/format.h"
#include "common/instruction.h"
#include "common/instruction_db.h"
#include "common/text_file.h"
#include "test_util.h"

using std::string;
using std::vector;

namespace sicxe {
namespace tests {

class InstructionDBTest : public testing::Test {
 protected:
  void TestLoadFromFile(const vector<string>& lines,
                        bool success, const string& errors,
                        InstructionDB* instruction_db) {
    TextFile file;
    file.set_file_name("test.txt");
    *file.mutable_lines() = lines;
    ErrorDB error_db;
    InstructionDB temp;
    InstructionDB* db_ptr = instruction_db == nullptr ? &temp : instruction_db;
    EXPECT_EQ(success, InstructionDB::LoadFromFile(file, db_ptr, &error_db));
    EXPECT_EQ(errors, TestUtil::ErrorDBToStringSimple(error_db));
  }
};

TEST_F(InstructionDBTest, OpcodeConflict) {
  InstructionDB db;
  db.Register(new Instruction(0x10, "ABC", Format::FS34, Syntax::FS34_LOAD_W));
  db.Register(new Instruction(0x20, "BCD", Format::F1,   Syntax::F1_NONE));
  db.Register(new Instruction(0x31, "CDE", Format::F2,   Syntax::F2_REG_REG));
  db.Register(new Instruction(0x33, "DEF", Format::F1,   Syntax::F1_NONE));
  db.Register(new Instruction(0x34, "EFG", Format::FS34, Syntax::FS34_LOAD_W));

  EXPECT_FALSE(db.IsOpcodeAvailable(0x51, Format::FS34));
  EXPECT_TRUE(db.IsOpcodeAvailable(0x50, Format::FS34));
  EXPECT_TRUE(db.IsOpcodeAvailable(0x51, Format::F2));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x12, Format::F1));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x10, Format::F1));
  EXPECT_TRUE(db.IsOpcodeAvailable(0x14, Format::F1));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x20, Format::FS34));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x20, Format::F1));
  EXPECT_TRUE(db.IsOpcodeAvailable(0x21, Format::F1));
  EXPECT_TRUE(db.IsOpcodeAvailable(0x22, Format::F1));
  EXPECT_TRUE(db.IsOpcodeAvailable(0x23, Format::F1));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x30, Format::FS34));
  EXPECT_TRUE(db.IsOpcodeAvailable(0x30, Format::F2));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x31, Format::F2));
  EXPECT_TRUE(db.IsOpcodeAvailable(0x32, Format::F2));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x33, Format::F2));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x34, Format::F2));
  EXPECT_FALSE(db.IsOpcodeAvailable(0x35, Format::F2));
}

TEST_F(InstructionDBTest, LoadFromFile) {
  TestLoadFromFile(vector<string>{
        "0x12 ADD TEST WHAT-IS-THIS",
      },
      false, "E[1:19:1];", nullptr);

  TestLoadFromFile(vector<string>{
        "0x12 ADD F1 NONE",
        "0x20 SUB F2 REG_REG TEST",
      },
      false, "E[2:21:4];", nullptr);

  TestLoadFromFile(vector<string>{
        "0x12    ADD    F1    NONE       ",
        "   0x20    SUB    F2        REG_REG",
        "0x10",
      },
      false, "E[3:4:1];", nullptr);

  TestLoadFromFile(vector<string>{
        "  BLA ADD F1 NONE",
      },
      false, "E[1:3:3];", nullptr);

  TestLoadFromFile(vector<string>{
        "  1000 ADD F1 NONE",
      },
      false, "E[1:3:4];", nullptr);

  TestLoadFromFile(vector<string>{
        "0x12 ADD F1 NONE",
        "    15 ADD_STH F1 NONE",
      },
      false, "E[2:8:7];", nullptr);

  TestLoadFromFile(vector<string>{
        "0x10 ADD F1 NONE",
        "0x14 ADD F2 REG_REG",
      },
      false, "E[2:6:3];", nullptr);

  TestLoadFromFile(vector<string>{
        "0x10 ADD TEST1 TEST1",
      },
      false, "E[1:10:5];", nullptr);

  TestLoadFromFile(vector<string>{
        "",
        "0x10 ADD F2 STORE_X",
      },
      false, "E[2:13:7];", nullptr);

  TestLoadFromFile(vector<string>{
        "0x10 ADD FS34 LOAD_W",
        "0x12 ADDR F2 REG_REG",
        "",
      },
      false, "E[2:1:4];", nullptr);

  InstructionDB db;
  TestLoadFromFile(vector<string>{
        "0x10  LDA    FS34   LOAD_W",
        "0x14  STA  \t  FS34   STORE_W",
        "",
        "0x18  ADDR   F2   REG_REG   ",
        "0x19  CLEAR  F1   NONE  ",
        " \t\t  ",
        "0x1a  SHIFTL\t F2  REG_N",
        "100   MOVR   F2  REG_REG",
      },
      true, "", &db);
  EXPECT_EQ("LDA", db.FindOpcode(0x10)->mnemonic());
  EXPECT_EQ(Format::FS34, db.FindMnemonic("STA")->format());
  EXPECT_EQ(0x19, db.FindMnemonic("CLEAR")->opcode());
  EXPECT_EQ(Syntax::F2_REG_N, db.FindOpcode(0x1a)->syntax());
  EXPECT_EQ(100, db.FindMnemonic("MOVR")->opcode());
}

}  // namespace tests
}  // namespace sicxe
