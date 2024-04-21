/*
VM.hpp
--------------------
Defines the VM class, containing the state of the script VM.
*/

#pragma once

#include "Script.hpp"
#include <array>
#include <nwge/common/array.hpp>

namespace nwge::proto::script {

enum class ErrorCode: u8 {
  OK,
  EndOfExecution, // the script ended execution
  IllegalInstr, // an illegal or unknown instruction was encountered
  InvalidReturn, // call stack underflow
  Unimplemented, // not yet implemented
};

struct Error {
  ErrorCode code = ErrorCode::OK;
  InstrPtr offset = 0;
};

class VM {
public:
  VM();
  ~VM();

  void load(const Script &script);
  void load(Script &&script);
  Error runNext();
  Error runAll();

private:
  /* constants, loaded from script */

  Script mScript{};

  /* runtime state */

  ComparisonResult mComparisonResult: 2 = ComparisonResult::Unknown;
  bool mConditionResult: 1 = false;
  InstrPtr mInstrPtr = 0;
  u8 mCallStackPtr = 0;
  std::array<Value, cRegisterCount> mRegisters{};
  std::array<Value, cSlotCount> mSlots{};
  std::array<InstrPtr, cCallStackSize> mCallStack{};

  /* helpers */

  Error runInstr(InstrS instr);
  Error runInstrEx(InstrExS instr);
};

} // namespace nwge::proto::script
