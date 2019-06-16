#pragma once

#include "Instructions.h"
#include "InstructionMacros.h"
#include <unordered_map>
#include <vector>
#include <stdint.h>

class VM;
class BytecodeBlock;
class Function;
class Register;
class Value;

class JIT {

public:
    using VirtualRegister = ::Register;
    enum Register : uint8_t;

    static void* compile(VM&, const BytecodeBlock&);

private:
    enum AddressTag { Address };
    struct Offset;
    struct Label;

    static Register defaultIndex;

    JIT(VM&, const BytecodeBlock&);

    VM* vm();
    void* compile();
    Label label();

    static Value trampoline(VM& vm, Function*, uint32_t argc, Value* argv);

    // HELPERS
    void prologue();
    void epilogue();

    // load(src, dst)
    void load(VirtualRegister, Register);
    void load(AddressTag, VirtualRegister, Register);

    // lea(src, dst)
    void lea(VirtualRegister, Register);

    // move(src, dst)
    void move(const void*, Register);
    void move(Value, Register);

    // store(dst, src);
    void store(Register, VirtualRegister);
    void store(Register, Register, int32_t);

    // calls
    template<typename T, typename Out, typename... In>
    void call(Out(T::*)(In...));

    template<typename T, typename Out, typename... In>
    void call(Out(T::*)(In...) const);

    template<typename Out, typename... In>
    void call(Out(*)(In...));

    template<typename Out, typename... In>
    void call(Out(*)(In&&...));

    // PRIMITIVES
    void move(Register, Register);
    void move(Offset, Register);
    void move(Register, Offset);
    void move(uint64_t, Register);
    void lea(Offset, Register);
    void call(void*);
    void compare(Register, Value);
    void compare(Register, Register);
    void setEqual(Register dst);
    void sub(Register, Register);
    void sub(int64_t, Register);
    void shiftl(uint8_t, Register);
    void bitAnd(int64_t, Register);
    void bitOr(int64_t, Register);
    void jump(int32_t);
    void jump(Label&);
    void jumpIfEqual(int32_t);
    void jumpIfEqual(Label&);
    void push(Register);
    void pop(Register);
    void ret();

    // WRITER
    void emitByte(uint8_t);
    void emitLong(uint32_t);
    void emitQuad(uint64_t);
    void emitJumpTarget(int32_t);
    void emitJumpTarget(Label&);
    void emitLabel(Label&);

#include "X64Primitives.h.inl"

#define DECLARE_OP(Instruction) void emit##Instruction(const Instruction&);
    FOR_EACH_INSTRUCTION(DECLARE_OP)
#undef DECLARE_OP

    VM& m_vm;
    const BytecodeBlock& m_block;
    uint32_t m_bytecodeOffset;
    std::vector<uint8_t> m_buffer;
    std::vector<std::pair<uint32_t, uint32_t>> m_jumps;
    std::unordered_map<uint32_t, uint32_t> m_bytecodeOffsetMapping;
};
