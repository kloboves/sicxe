#include "common/instruction_db.h"

#include "common/format.h"
#include "common/instruction.h"
#include "common/opcode.h"

namespace sicxe {

namespace {

InstructionDB* CreateDefaultInstance() {
  InstructionDB* db = new InstructionDB;
  typedef Instruction Insn;

  // standard instruction set
  db->Register(new Insn(Opcode::CLEAR,  "CLEAR",   Format::F2,    Syntax::F2_REG));
  db->Register(new Insn(Opcode::RMO,    "RMO",     Format::F2,    Syntax::F2_REG_REG));

  db->Register(new Insn(Opcode::ADD,    "ADD",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::SUB,    "SUB",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::MUL,    "MUL",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::DIV,    "DIV",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::AND,    "AND",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::OR,     "OR",      Format::FS34,  Syntax::FS34_LOAD_W));

  db->Register(new Insn(Opcode::ADDR,   "ADDR",    Format::F2,    Syntax::F2_REG_REG));
  db->Register(new Insn(Opcode::SUBR,   "SUBR",    Format::F2,    Syntax::F2_REG_REG));
  db->Register(new Insn(Opcode::MULR,   "MULR",    Format::F2,    Syntax::F2_REG_REG));
  db->Register(new Insn(Opcode::DIVR,   "DIVR",    Format::F2,    Syntax::F2_REG_REG));

  db->Register(new Insn(Opcode::SHIFTL, "SHIFTL",  Format::F2,    Syntax::F2_REG_N));
  db->Register(new Insn(Opcode::SHIFTR, "SHIFTR",  Format::F2,    Syntax::F2_REG_N));

  db->Register(new Insn(Opcode::COMP,   "COMP",    Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::COMPR,  "COMPR",   Format::F2,    Syntax::F2_REG_REG));

  db->Register(new Insn(Opcode::TIX,    "TIX",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::TIXR,   "TIXR",    Format::F2,    Syntax::F2_REG));

  db->Register(new Insn(Opcode::J,      "J",       Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::JEQ,    "JEQ",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::JGT,    "JGT",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::JLT,    "JLT",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::JSUB,   "JSUB",    Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::RSUB,   "RSUB",    Format::FS34,  Syntax::FS34_NONE));

  db->Register(new Insn(Opcode::LDCH,   "LDCH",    Format::FS34,  Syntax::FS34_LOAD_B));
  db->Register(new Insn(Opcode::STCH,   "STCH",    Format::FS34,  Syntax::FS34_STORE_B));
  db->Register(new Insn(Opcode::STSW,   "STSW",    Format::FS34,  Syntax::FS34_STORE_B));

  db->Register(new Insn(Opcode::LDA,    "LDA",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::LDB,    "LDB",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::LDL,    "LDL",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::LDS,    "LDS",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::LDT,    "LDT",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::LDX,    "LDX",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::STA,    "STA",     Format::FS34,  Syntax::FS34_STORE_W));
  db->Register(new Insn(Opcode::STB,    "STB",     Format::FS34,  Syntax::FS34_STORE_W));
  db->Register(new Insn(Opcode::STL,    "STL",     Format::FS34,  Syntax::FS34_STORE_W));
  db->Register(new Insn(Opcode::STS,    "STS",     Format::FS34,  Syntax::FS34_STORE_W));
  db->Register(new Insn(Opcode::STT,    "STT",     Format::FS34,  Syntax::FS34_STORE_W));
  db->Register(new Insn(Opcode::STX,    "STX",     Format::FS34,  Syntax::FS34_STORE_W));

  db->Register(new Insn(Opcode::TD,     "TD",      Format::FS34,  Syntax::FS34_LOAD_B));
  db->Register(new Insn(Opcode::RD,     "RD",      Format::FS34,  Syntax::FS34_LOAD_B));
  db->Register(new Insn(Opcode::WD,     "WD",      Format::FS34,  Syntax::FS34_LOAD_B));

  db->Register(new Insn(Opcode::FIX,    "FIX",     Format::F1,    Syntax::F1_NONE));
  db->Register(new Insn(Opcode::FLOAT,  "FLOAT",   Format::F1,    Syntax::F1_NONE));

  db->Register(new Insn(Opcode::ADDF,   "ADDF",    Format::FS34,  Syntax::FS34_LOAD_F));
  db->Register(new Insn(Opcode::SUBF,   "SUBF",    Format::FS34,  Syntax::FS34_LOAD_F));
  db->Register(new Insn(Opcode::MULF,   "MULF",    Format::FS34,  Syntax::FS34_LOAD_F));
  db->Register(new Insn(Opcode::DIVF,   "DIVF",    Format::FS34,  Syntax::FS34_LOAD_F));
  db->Register(new Insn(Opcode::COMPF,  "COMPF",   Format::FS34,  Syntax::FS34_LOAD_F));

  db->Register(new Insn(Opcode::LDF,    "LDF",     Format::FS34,  Syntax::FS34_LOAD_F));
  db->Register(new Insn(Opcode::STF,    "STF",     Format::FS34,  Syntax::FS34_STORE_F));

  // extended instruction set
  db->Register(new Insn(Opcode::XOR,    "XOR",     Format::FS34,  Syntax::FS34_LOAD_W));
  db->Register(new Insn(Opcode::ANDR,   "ANDR",    Format::F2,    Syntax::F2_REG_REG));
  db->Register(new Insn(Opcode::ORR,    "ORR",     Format::F2,    Syntax::F2_REG_REG));
  db->Register(new Insn(Opcode::XORR,   "XORR",    Format::F2,    Syntax::F2_REG_REG));
  db->Register(new Insn(Opcode::NOT,    "NOT",     Format::F2,    Syntax::F2_REG));

  db->Register(new Insn(Opcode::EINT,   "EINT",    Format::F1,    Syntax::F1_NONE));
  db->Register(new Insn(Opcode::DINT,   "DINT",    Format::F1,    Syntax::F1_NONE));
  db->Register(new Insn(Opcode::RINT,   "RINT",    Format::F1,    Syntax::F1_NONE));
  db->Register(new Insn(Opcode::STIL,   "STIL",    Format::FS34,  Syntax::FS34_STORE_W));

  return db;
}

}  // namespace

const InstructionDB* InstructionDB::Default() {
  static InstructionDB* instance = CreateDefaultInstance();
  return instance;
}

}  // namespace sicxe
