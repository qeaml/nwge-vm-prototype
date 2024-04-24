#include <nwge/engine.hpp>
#include "script/VM.hpp"

using namespace nwge;

class Test: public nwge::State {
public:
  bool init() override {
    mVM.bind(10, [](auto &value){
      infoBox("Hello from slot 10!",
        "Slot 10 is now {}", value);
    });

    mVM.registerFunc("Hello",
      []([[maybe_unused]] const auto &args){
        infoBox("Hello",
          "Hello from external function");
        return 0;
      });

    auto errorCode = mVM.load(proto::script::Script{
      {ArrayView<const String<>>{
        "Hello",
      }},
      {},
      {},
      {ArrayView<const proto::script::Instr>{
        SCRIPT_INSTR_EX(proto::script::Opcode::LdI, 3, 100),
        SCRIPT_INSTR_EX(proto::script::Opcode::MvR2S, 3, 10),
        SCRIPT_INSTR_EX(proto::script::Opcode::XCall, 3, 0),
      }}
    });
    if(errorCode != proto::script::ErrorCode::OK) {
      errorBox("Script Error",
        "Failed to load script: {}",
        errMsg(errorCode));
      return false;
    }

    auto err = mVM.runAll();
    if(err.code != proto::script::ErrorCode::OK) {
      errorBox("Script Error",
        "At offset {:06x}:\n"
        "{}",
        err.offset, errMsg(err.code));
      return false;
    }

    return true;
  }

private:
  proto::script::VM mVM;
};

s32 main([[maybe_unused]] s32 argc, [[maybe_unused]] CStr *argv) {
  start<Test>({
    .appName = "VM Prototype",
    .windowAspectRatio = {4, 3},
  });
  return 0;
}
