/*
Script.hpp
--------------------
A Script loaded from disk
*/

#pragma once

#include "types.hpp"
#include <nwge/common/array.hpp>
#include <nwge/common/string.hpp>

namespace nwge::proto::script {

struct Script {
  Array<String<>> symbolTable{};
  Array<Value> constTable{};
  Array<InstrPtr> markerTable{};
  Array<Instr> code{};
};

} // namespace nwge::proto::script
