#include "script/VM.hpp"
#include <utility>

namespace nwge::proto::script {

VM::VM() = default;

VM::~VM() = default;

Value &VM::bind(Slot slot, SlotCallback callback) {
  auto &slotData = mSlots[slot];
  slotData.callback = std::move(callback);
  return slotData.value;
}

void VM::registerFunc(const StringView &name, FuncCallback callback) {
  mFuncDefs.push({name, std::move(callback)});
}

ErrorCode VM::load(const Script &script) {
  mScript = script;
  return loadScript();
}

ErrorCode VM::load(Script &&script) {
  mScript = std::move(script);
  return loadScript();
}

ErrorCode VM::loadScript() {
  mInstrPtr = 0;
  mCallStack.fill(cInvalidInstrPtr);
  mCallStackPtr = 0;
  mComparisonResult = ComparisonResult::Unknown;
  mConditionResult = false;
  mRegisters.fill(0);
  mSlots.fill({});

  if(!resolveSymbols()) {
    return ErrorCode::MissingSymbol;
  }

  return ErrorCode::OK;
}

bool VM::resolveSymbols() {
  mFuncTable.fill(nullptr);
  for(usize i = 0; i < mScript.symbolTable.size(); ++i) {
    auto name = mScript.symbolTable[i].view();
    for(auto &funcDef : mFuncDefs) {
      if(funcDef.name == name) {
        mFuncTable[i] = &funcDef.callback;
        goto next;
      }
    }
    return false;
    next:;
  }
  return true;
}

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
    const auto &slot = mSlots[instr.arg];
    mConditionResult = slot.value != 0;
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
    mRegisters[instr.reg] = mSlots[instr.arg].value;
    break;
  }

  case Opcode::MvR2S: {
    auto &slot = mSlots[instr.arg];
    auto value = mRegisters[instr.reg];
    slot.value = value;
    if(slot.callback != nullptr) {
      slot.callback(slot.value);
    }
    break;
  }

  case Opcode::LdI: {
    mRegisters[instr.reg] = instr.arg;
    break;
  }

  case Opcode::LdC: {
    mRegisters[instr.reg] = mScript.constTable[instr.arg];
    break;
  }

  case Opcode::XCall: {
    auto *callback = mFuncTable[instr.arg];
    if(callback == nullptr) {
      return {ErrorCode::IllegalXCall, mInstrPtr};
    }
    ArrayView<Value> args{mRegisters.data(), cRegisterCount};
    mRegisters[0] = (*callback)(args.sub(instr.reg));
    return {};
  }

  default:
    return {ErrorCode::IllegalInstr, mInstrPtr};

  }

  return {};
}

static constexpr std::array<const StringView, usize(ErrorCode::Max)>
  cErrMsgs{
    "OK",
    "the script ended execution",
    "an illegal or unknown instruction was encountered",
    "call stack underflow",
    "call to unknown external function",
    "a function in the symbol table was not found",
    "not yet implemented",
  };

const StringView &errMsg(ErrorCode code) {
  return cErrMsgs[usize(code)];
}

} // namespace nwge::proto::script
