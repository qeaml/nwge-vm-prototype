#include <nwge/engine.hpp>
#include "script/VM.hpp"

using namespace nwge;

class Test: public nwge::State {
public:
  bool init() override {
    console::print("The script must be now loaded... TODO!");
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
