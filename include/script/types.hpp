/*
types.h
--------------------
Script VM type definitions
*/

#pragma once

#include <nwge/common/def.h>

namespace nwge::proto::script {

using Value = s16;

using Register = u8;
[[maybe_unused]]
static constexpr usize cRegisterCount = 16;
[[maybe_unused]]
static constexpr u8 cRegisterMask = 0xF;

using Slot = u8;
[[maybe_unused]]
static constexpr usize cSlotCount = 256;

using MarkerIdx = u8;
using Instr = u16;
using InstrPtr = u16;
[[maybe_unused]]
static constexpr InstrPtr cInvalidInstrPtr = 0xFFFF;

[[maybe_unused]]
static constexpr usize cCallStackSize = 16;

#define SCRIPT_INSTR_OPCODE(instr) (u8)(((instr) >> 8) & 0xFF)
#define SCRIPT_INSTR_ARG(instr) (u8)((instr) & 0xFF)

#define SCRIPT_INSTR_IS_EX(instr) (((instr) & 0x8000) == 0x8000)
#define SCRIPT_INSTR_EX_OPCODE(instr) (u8)(((instr) >> 12) & 0xF)
#define SCRIPT_INSTR_EX_REG(instr) (u8)(((instr) >> 8) & 0xF)
#define SCRIPT_INSTR_EX_ARG(instr) (u8)((instr) & 0xFF)

enum class Opcode: u8 {
  Nop,
  /// Move from a register to another
  MvR2R,
  /// Compare two registers
  Cmp,
  /// Check if a register is non-zero
  Test,
  /// Check if a slot is non-zero
  Bool,
  /// Jump to a marker
  Jmp,
  /// Jump to an 8-bit offset from beginning of code
  JSA,
  /// Jump to an 8-bit offset from the current instruction
  JSR,
  /// Jump to a marker if the comparison result is Lesser
  JL,
  /// Jump to a marker if the comparison result is Equal
  JE,
  /// Jump to a marker if the comparison result is Greater
  JG,
  /// Jump to a marker if the comparison result is Not Equal
  JNE,
  /// Jump to a marker if the comparison result is Greater or Equal
  JGE,
  /// Jump to a marker if the comparison result is Lesser or Equal
  JLE,
  /// Jump to a marker if the condition result is true
  JT,
  /// Jump to a marker if the condition result is false
  JF,
  /// Perform a call to a marker (push return address)
  Call,
  /// Perform a call to an 8-bit offset from beginning of code
  CallSA,
  /// Perform a call to an 8-bit offset from the current instruction
  CallSR,
  /// Return from a call
  Ret,

  /// Move from a slot to a register
  MvS2R = 1 << 7, // <-- highest bit set = extended argument encoding
  /// Move from a register to a slot
  MvR2S,
  /// Load an 8-bit value into a slot
  LdI,
  /// Load a 16-bit value into a slot
  LdC,
  /// Call an external function
  XCall,
};

struct InstrS {
  u8 op;
  u8 arg;

  static constexpr inline InstrS decode(Instr raw) {
    return {
      SCRIPT_INSTR_OPCODE(raw),
      SCRIPT_INSTR_ARG(raw)
    };
  }
};

struct InstrExS { // extended argument encoding
  u8 op: 4;
  u8 reg: 4;
  u8 arg;

  static constexpr inline InstrExS decode(Instr raw) {
    return {
      SCRIPT_INSTR_EX_OPCODE(raw),
      SCRIPT_INSTR_EX_REG(raw),
      SCRIPT_INSTR_EX_ARG(raw)
    };
  }
};

NWGE_STATIC_ASSERT(sizeof(InstrExS) == sizeof(InstrS),
  "InstrEx must be same size as Instr");

enum class ComparisonResult: u8 {
  Unknown,
  Lesser,
  Equal,
  Greater
};

} // namespace nwge::proto::script
