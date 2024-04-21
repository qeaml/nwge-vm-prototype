#include "script/VM.hpp"

namespace nwge::proto::script {

VM::VM() {
  mCallStack.fill(cInvalidInstrPtr);
}

VM::~VM() = default;

Error VM::runAll() {
  Error err;
  while((err = runNext()).code == ErrorCode::OK) {}
  if(err.code == ErrorCode::EndOfExecution) {
    err.code = ErrorCode::OK;
  }
  return err;
}

Error VM::runNext() {
  if(mInstrPtr >= mScript.code.size()) {
    return {ErrorCode::EndOfExecution, mInstrPtr};
  }

  u16 instr = mScript.code[mInstrPtr];
  Error err;
  if(SCRIPT_INSTR_IS_EX(instr)) {
    err = runInstrEx(InstrExS::decode(instr));
  } else {
    err = runInstr(InstrS::decode(instr));
  }
  if(err.code == ErrorCode::OK) {
    ++mInstrPtr;
  }
  return err;
}

Error VM::runInstr(InstrS instr) {
  switch(Opcode(instr.op)) {
  case Opcode::Nop:
    break;

  case Opcode::MvR2R: {
    auto regDst = Register((instr.arg >> 4) & cRegisterMask);
    auto regSrc = Register(instr.arg & cRegisterMask);
    mRegisters[regDst] = mRegisters[regSrc];
    break;
  }

  case Opcode::Cmp: {
    auto regA = Register((instr.arg >> 4) & cRegisterMask);
    auto regB = Register(instr.arg & cRegisterMask);
    auto valA = mRegisters[regA];
    auto valB = mRegisters[regB];
    if(valA < valB) {
      mComparisonResult = ComparisonResult::Lesser;
    } else if(valA > valB) {
      mComparisonResult = ComparisonResult::Greater;
    } else {
      mComparisonResult = ComparisonResult::Equal;
    }
    mConditionResult = valA == valB;
    break;
  }

  case Opcode::Test: {
    auto reg = Register((instr.arg >> 4) & cRegisterMask);
    auto val = mRegisters[reg];
    mConditionResult = val != 0;
    break;
  }

  case Opcode::Bool: {
    auto slot = Slot(instr.arg);
    auto val = mSlots[slot];
    mConditionResult = val != 0;
    break;
  }

  case Opcode::Jmp: {
    auto markerIdx = MarkerIdx(instr.arg);
    mInstrPtr = mScript.markerTable[markerIdx];
    break;
  }

  case Opcode::JSA: {
    auto offset = InstrPtr(instr.arg);
    mInstrPtr = offset;
    break;
  }

  case Opcode::JSR: {
    auto offset = InstrPtr(instr.arg);
    mInstrPtr += offset;
    break;
  }

  case Opcode::JL: {
    auto markerIdx = MarkerIdx(instr.arg);
    if(mComparisonResult == ComparisonResult::Lesser) {
      mInstrPtr = mScript.markerTable[markerIdx];
    }
    break;
  }

  case Opcode::JE: {
    auto markerIdx = MarkerIdx(instr.arg);
    if(mComparisonResult == ComparisonResult::Equal) {
      mInstrPtr = mScript.markerTable[markerIdx];
    }
    break;
  }

  case Opcode::JG: {
    auto markerIdx = MarkerIdx(instr.arg);
    if(mComparisonResult == ComparisonResult::Greater) {
      mInstrPtr = mScript.markerTable[markerIdx];
    }
    break;
  }

  case Opcode::JNE: {
    auto markerIdx = MarkerIdx(instr.arg);
    if(mComparisonResult != ComparisonResult::Equal) {
      mInstrPtr = mScript.markerTable[markerIdx];
    }
    break;
  }

  case Opcode::JLE: {
    auto markerIdx = MarkerIdx(instr.arg);
    if(mComparisonResult == ComparisonResult::Lesser ||
      (mComparisonResult == ComparisonResult::Equal)) {
      mInstrPtr = mScript.markerTable[markerIdx];
    }
    break;
  }

  case Opcode::JGE: {
    auto markerIdx = MarkerIdx(instr.arg);
    if(mComparisonResult == ComparisonResult::Greater ||
      (mComparisonResult == ComparisonResult::Equal)) {
      mInstrPtr = mScript.markerTable[markerIdx];
    }
    break;
  }

  case Opcode::JT: {
    auto markerIdx = MarkerIdx(instr.arg);
    if(mConditionResult) {
      mInstrPtr = mScript.markerTable[markerIdx];
    }
    break;
  }

  case Opcode::JF: {
    auto markerIdx = MarkerIdx(instr.arg);
    if(!mConditionResult) {
      mInstrPtr = mScript.markerTable[markerIdx];
    }
    break;
  }

  case Opcode::Call: {
    auto markerIdx = MarkerIdx(instr.arg);
    mCallStack[mCallStackPtr++] = mInstrPtr;
    mInstrPtr = mScript.markerTable[markerIdx];
    break;
  }

  case Opcode::CallSA: {
    auto instrPtr = InstrPtr(instr.arg);
    mCallStack[mCallStackPtr++] = mInstrPtr;
    mInstrPtr = instrPtr;
    break;
  }

  case Opcode::CallSR: {
    InstrPtr instrPtr = mInstrPtr + instr.arg;
    mCallStack[mCallStackPtr++] = mInstrPtr;
    mInstrPtr = instrPtr;
    break;
  }

  case Opcode::Ret: {
    if(mCallStackPtr == 0) {
      return {ErrorCode::InvalidReturn, mInstrPtr};
    }
    mInstrPtr = mCallStack[--mCallStackPtr];
    break;
  }

  default:
    return {ErrorCode::IllegalInstr, mInstrPtr};

  }

  return {};
}

Error VM::runInstrEx(InstrExS instr) {
  switch(Opcode(instr.op)) {
  case Opcode::MvS2R: {
    auto slot = Slot(instr.arg);
    auto reg = Register(instr.reg);
    mRegisters[reg] = mSlots[slot];
    break;
  }

  case Opcode::MvR2S: {
    auto slot = Slot(instr.arg);
    auto reg = Register(instr.reg);
    mSlots[slot] = mRegisters[reg];
    break;
  }

  case Opcode::LdI: {
    auto reg = Register(instr.reg);
    auto val = instr.arg;
    mRegisters[reg] = val;
    break;
  }

  case Opcode::LdC: {
    auto reg = Register(instr.reg);
    auto idx = instr.arg;
    mRegisters[reg] = mScript.constTable[idx];
    break;
  }

  case Opcode::XCall: {
    // auto retReg = Register(instr.reg);
    // auto idx = instr.arg;
    // TODO: call external function idx and store return value in retReg
    return {ErrorCode::Unimplemented, mInstrPtr};
  }

  default:
    return {ErrorCode::IllegalInstr, mInstrPtr};

  }

  return {};
}

} // namespace nwge::proto::script