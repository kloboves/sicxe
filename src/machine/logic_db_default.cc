#include "machine/logic_db.h"

#include "common/cpu_state.h"
#include "common/opcode.h"
#include "machine/logic.h"
#include "machine/logic/arithmetic.h"
#include "machine/logic/compare.h"
#include "machine/logic/device.h"
#include "machine/logic/float.h"
#include "machine/logic/interrupt.h"
#include "machine/logic/jump.h"
#include "machine/logic/load.h"
#include "machine/logic/move.h"
#include "machine/logic/shift.h"
#include "machine/logic/store.h"

namespace sicxe {
namespace machine {

namespace logic {
namespace {

LogicDB* CreateDefaultInstance() {
  LogicDB* db = new LogicDB;

  // standard instruction set
  db->Register(Opcode::CLEAR, new ClearReg());
  db->Register(Opcode::RMO, new MoveReg());

  db->Register(Opcode::ADD, new ArithmeticMem(ArithmeticOperation::ADD));
  db->Register(Opcode::SUB, new ArithmeticMem(ArithmeticOperation::SUBTRACT));
  db->Register(Opcode::MUL, new ArithmeticMem(ArithmeticOperation::MULTIPLY));
  db->Register(Opcode::DIV, new ArithmeticMem(ArithmeticOperation::DIVIDE));
  db->Register(Opcode::AND, new ArithmeticMem(ArithmeticOperation::AND));
  db->Register(Opcode::OR, new ArithmeticMem(ArithmeticOperation::OR));

  db->Register(Opcode::ADDR, new ArithmeticReg(ArithmeticOperation::ADD));
  db->Register(Opcode::SUBR, new ArithmeticReg(ArithmeticOperation::SUBTRACT));
  db->Register(Opcode::MULR, new ArithmeticReg(ArithmeticOperation::MULTIPLY));
  db->Register(Opcode::DIVR, new ArithmeticReg(ArithmeticOperation::DIVIDE));

  db->Register(Opcode::SHIFTL, new Shift(false));
  db->Register(Opcode::SHIFTR, new Shift(true));

  db->Register(Opcode::COMP, new CompareMem);
  db->Register(Opcode::COMPR, new CompareReg);

  db->Register(Opcode::TIX, new IncrementCompareMem);
  db->Register(Opcode::TIXR, new IncrementCompareReg);

  db->Register(Opcode::J, new Jump(false));
  db->Register(Opcode::JEQ, new JumpConditional(CpuState::EQUAL));
  db->Register(Opcode::JGT, new JumpConditional(CpuState::GREATER));
  db->Register(Opcode::JLT, new JumpConditional(CpuState::LESS));
  db->Register(Opcode::JSUB, new Jump(true));
  db->Register(Opcode::RSUB, new Return);

  db->Register(Opcode::LDCH, new LoadByte);
  db->Register(Opcode::STCH, new StoreByte);
  db->Register(Opcode::STSW, new StoreFlags);

  db->Register(Opcode::LDA, new LoadWord(CpuState::REG_A));
  db->Register(Opcode::LDB, new LoadWord(CpuState::REG_B));
  db->Register(Opcode::LDL, new LoadWord(CpuState::REG_L));
  db->Register(Opcode::LDS, new LoadWord(CpuState::REG_S));
  db->Register(Opcode::LDT, new LoadWord(CpuState::REG_T));
  db->Register(Opcode::LDX, new LoadWord(CpuState::REG_X));
  db->Register(Opcode::STA, new StoreWord(CpuState::REG_A));
  db->Register(Opcode::STB, new StoreWord(CpuState::REG_B));
  db->Register(Opcode::STL, new StoreWord(CpuState::REG_L));
  db->Register(Opcode::STS, new StoreWord(CpuState::REG_S));
  db->Register(Opcode::STT, new StoreWord(CpuState::REG_T));
  db->Register(Opcode::STX, new StoreWord(CpuState::REG_X));

  db->Register(Opcode::TD,  new TestDevice);
  db->Register(Opcode::RD,  new ReadDevice);
  db->Register(Opcode::WD,  new WriteDevice);

  db->Register(Opcode::FIX, new FloatToInt);
  db->Register(Opcode::FLOAT, new IntToFloat);

  db->Register(Opcode::ADDF, new FloatArithmetic(FloatArithmetic::ADD));
  db->Register(Opcode::SUBF, new FloatArithmetic(FloatArithmetic::SUBTRACT));
  db->Register(Opcode::MULF, new FloatArithmetic(FloatArithmetic::MULTIPLY));
  db->Register(Opcode::DIVF, new FloatArithmetic(FloatArithmetic::DIVIDE));
  db->Register(Opcode::COMPF, new FloatCompare);

  db->Register(Opcode::LDF, new LoadFloat);
  db->Register(Opcode::STF, new StoreFloat);

  // extended instruction set
  db->Register(Opcode::XOR, new ArithmeticMem(ArithmeticOperation::XOR));
  db->Register(Opcode::ANDR, new ArithmeticReg(ArithmeticOperation::AND));
  db->Register(Opcode::ORR, new ArithmeticReg(ArithmeticOperation::OR));
  db->Register(Opcode::XORR, new ArithmeticReg(ArithmeticOperation::XOR));
  db->Register(Opcode::NOT, new RegisterNegate);

  db->Register(Opcode::EINT, new InterruptEnable);
  db->Register(Opcode::DINT, new InterruptDisable);
  db->Register(Opcode::RINT, new InterruptReturn);
  db->Register(Opcode::STIL, new InterruptLinkStore);

  return db;
}

}  // namespace
}  // namespace logic

const LogicDB* LogicDB::Default() {
  static LogicDB* instance = logic::CreateDefaultInstance();
  return instance;
}

}  // namespace machine
}  // namespace sicxe
