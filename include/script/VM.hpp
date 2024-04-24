/*
VM.hpp
--------------------
Defines the VM class, containing the state of the script VM.
*/

#pragma once

#include "Script.hpp"
#include <array>
#include <functional>
#include <nwge/common/array.hpp>
#include <nwge/common/slice.hpp>
#include <nwge/common/string.hpp>

namespace nwge::proto::script {

enum class ErrorCode: u8 {
  OK,
  EndOfExecution, // the script ended execution
  IllegalInstr,   // an illegal or unknown instruction was encountered
  InvalidReturn,  // call stack underflow
  IllegalXCall,   // call to unknown external function
  MissingSymbol,  // a function in the symbol table was not found
  Unimplemented,  // not yet implemented
  Max,
};

const StringView &errMsg(ErrorCode code);

struct Error {
  ErrorCode code = ErrorCode::OK;
  InstrPtr offset = 0;
};

class VM {
public:
  VM();
  ~VM();

  ErrorCode load(const Script &script);
  ErrorCode load(Script &&script);
  Error runNext();
  Error runAll();

  using SlotCallback = std::function<void(Value&)>;

  Value &bind(Slot slot, SlotCallback callback = nullptr);

  using FuncCallback = std::function<Value(const ArrayView<Value>&)>;

  void registerFunc(const StringView &name, FuncCallback callback);

private:
  /* constants, loaded from script */

  Script mScript{};
  ErrorCode loadScript();

  struct FuncDef {
    String<> name;
    FuncCallback callback;
  };
  /// the function definitions specified by host app
  Slice<FuncDef> mFuncDefs{cDefaultFuncDefCount};
  /// function table used during runtime
  std::array<FuncCallback*, cFuncTableSize> mFuncTable{};
  bool resolveSymbols();

  /* runtime state */

  ComparisonResult mComparisonResult: 2 = ComparisonResult::Unknown;
  bool mConditionResult: 1 = false;
  InstrPtr mInstrPtr = 0;
  u8 mCallStackPtr = 0;
  std::array<Value, cRegisterCount> mRegisters{};
  std::array<InstrPtr, cCallStackSize> mCallStack{};

  struct SlotData {
    Value value = 0;
    SlotCallback callback = nullptr;
  };
  std::array<SlotData, cSlotCount> mSlots{};

  /* helpers */

  Error runInstr(InstrS instr);
  Error runInstrEx(InstrExS instr);
};

} // namespace nwge::proto::script
