# Nwge VM Prototype

This is a simple prototype for a scripting VM to be added to the nwge engine.

The engine previously had a scripting system, also featuring a VM. That previous
implementation was... a mess. This is a rewrite of that VM, with a focus on
making it simpler but equally as powerful.

## Main differences

* The old VM was primarily 32-bit. This implementation is limited entirely to 16
  bits.
* The old VM had only 256 slots. This implementation has 16 registers and 256
  slots. These extra registers are necessitated by the new 16-bit instruction
  encoding.
* The old VM stored types alongside its values. In this implementation, all
  values are signed 16-bit integers. The new implementation also features status
  flags for conditional logic as opposed to the previous VM using boolean
  values.
* The old VM had a separate value type for strings. This implementation does not
  handle strings in itself. The best way to implement strings is to use 16-bit
  string indices rather than strings directly.
* The old VM had no arithmetic instructions. This implementation features all
  the basic instructions: add, subtract, multiply, divide etc.
* The old VM had a separate `float` type. This implementation does not support
  *any* floating-point arithmetic. You must implement your own integer->float
  conversion scheme yourself or simply avoid having the VM perform any
  arithmetic.
* The old VM quit with no error message if a malformed instruction or invalid
  instruction pointer was executed. This implementation features more detailed
  error reporting through an error code paired with the offset at which the
  error arose. This allows for better debugging.
* The old VM had a separate concept of "external variables" registered by the
  host app and referenced by the script. This implementation will instead use
  bound slots, where a slot is bound to a value held by the host app. The host
  app, just like the previous iteration's variables, is notified every time that
  value changes.
* This implementation features many more variations of the `jmp` (and `call`)
  instruction, to help avoid using up too many markers. When a jump (or call) is
  small enough, then only the difference of the instruction pointer between the
  jump and the destination is used rather than a full-blown address in the
  marker table. This helps as marker indices are stored as 8-bit integers,
  rather than the previous 16-bit.
* Calling external functions is slightly different. The script now defines which
  register the function arguments start at, and the returned result is always
  stored in register 0.
