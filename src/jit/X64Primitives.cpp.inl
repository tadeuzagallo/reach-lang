#include "JIT.h"
#include <stdint.h>

enum X86GPRs {
    rax,
    rcx,
    rdx,
    rbx,
    rsp,
    rbp,
    rsi,
    rdi,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15
};

enum JIT::Register : uint8_t {
    regA0 = rdi,
    regA1 = rsi,
    regA2 = rdx,
    regA3 = rcx,

    regR0 = rax,

    regT0 = rdi,
    regT1 = rsi,
    regT2 = rdx,

    regCFR = rbp,
    regSP = rsp,
};

static constexpr JIT::Register tmpRegister = static_cast<JIT::Register>(r8);

namespace REX {
static constexpr JIT::Register NoR = static_cast<JIT::Register>(0);
static constexpr JIT::Register NoX = static_cast<JIT::Register>(0);
static constexpr JIT::Register NoB = static_cast<JIT::Register>(0);
};

namespace SIB {
static constexpr JIT::Register NoIndex = static_cast<JIT::Register>(rsp);
static constexpr JIT::Register HasSib = static_cast<JIT::Register>(rsp);
};

JIT::Register JIT::defaultIndex = SIB::NoIndex;

static constexpr bool is8Bit(int32_t i)
{
    return i == static_cast<int32_t>(static_cast<int8_t>(i));
}

enum JIT::Opcode : uint8_t {
    OP_MOV_EvGv = 0x89,
    OP_MOV_GvEv = 0x8B,
    OP_MOV_EAXIv = 0xB8,
    OP_2BYTE_ESCAPE = 0x0F,
    OP_GROUP5_Ev = 0xFF,
    OP_CMP_EvGv = 0x39,
    OP_SUB_EvGv = 0x29,
    OP_JMP_rel32 = 0xE9,
    OP_LEA = 0x8D,
    OP_PUSH_EAX = 0x50,
    OP_POP_EAX = 0x58,
    OP_RET = 0xC3,
    OP_GROUP2_EvIb = 0xC1,
    OP_AND_EvGv = 0x21,
};

static constexpr uint8_t OP2_JCC_rel32 = 0x80;
static constexpr uint8_t GROUP5_OP_CALLN = 0x2;
static constexpr uint8_t GROUP2_OP_SHL = 0x4;
static constexpr uint8_t ConditionE = 0x4;

enum class JIT::ModRM : uint8_t {
    None,
    Offset8Bit,
    Offset32Bit,
    Register,
};

void JIT::move(Register src, Register dst)
{
    emitRex(src, REX::NoX, dst);
    emitOpcode(OP_MOV_EvGv);
    emitModRm(ModRM::Register, src, dst);
}

void JIT::move(Offset offset, Register dst)
{
    move(OP_MOV_GvEv, dst, offset);
}

void JIT::move(Register src, Offset offset)
{
    move(OP_MOV_EvGv, src, offset);
}

void JIT::move(uint64_t immediate, Register dst)
{
    emitRex(REX::NoR, REX::NoX, dst);
    emitOpcode(OP_MOV_EAXIv, dst);
    emitQuad(immediate);
}

void JIT::lea(Offset offset, Register dst)
{
    move(OP_LEA, dst, offset);
}

void JIT::call(void* target)
{
    move(target, tmpRegister);
    emitRex(REX::NoR, REX::NoX, tmpRegister);
    emitOpcode(OP_GROUP5_Ev);
    emitModRm(ModRM::Register, GROUP5_OP_CALLN, tmpRegister);
}

void JIT::compare(Register reg, Value value)
{
    move(value.m_bits, tmpRegister);
    emitRex(tmpRegister, REX::NoX, reg);
    emitOpcode(OP_CMP_EvGv);
    emitModRm(ModRM::Register, tmpRegister, reg);
}

void JIT::sub(int64_t immediate, Register reg)
{
    move(immediate, tmpRegister);
    sub(tmpRegister, reg);
}

void JIT::sub(Register lhs, Register rhs)
{
    emitRex(lhs, REX::NoX, rhs);
    emitOpcode(OP_SUB_EvGv);
    emitModRm(ModRM::Register, lhs, rhs);
}

void JIT::shiftl(uint8_t immediate, Register reg)
{
    emitRex(REX::NoR, REX::NoX, reg);
    emitOpcode(OP_GROUP2_EvIb);
    emitModRm(ModRM::Register, GROUP2_OP_SHL, reg);
    emitByte(immediate);
}

void JIT::bit_and(int64_t immediate, Register reg)
{
    move(immediate, tmpRegister);
    emitRex(tmpRegister, REX::NoX, reg);
    emitOpcode(OP_AND_EvGv);
    emitModRm(ModRM::Register, tmpRegister, reg);
}

// jumps
void JIT::jump(int32_t target)
{
    emitOpcode(OP_JMP_rel32);
    emitJumpTarget(target);
}

void JIT::jump(Label& target)
{
    emitOpcode(OP_JMP_rel32);
    emitJumpTarget(target);
}

void JIT::jumpIfEqual(int32_t target)
{
    emitOpcode(OP_2BYTE_ESCAPE);
    emitOpcode(OP2_JCC_rel32, ConditionE);
    emitJumpTarget(target);
}

void JIT::jumpIfEqual(Label& target)
{
    emitOpcode(OP_2BYTE_ESCAPE);
    emitOpcode(OP2_JCC_rel32, ConditionE);
    emitJumpTarget(target);
}

void JIT::push(Register reg)
{
    emitOpcode(OP_PUSH_EAX, reg);
}

void JIT::pop(Register reg)
{
    emitOpcode(OP_POP_EAX, reg);
}

void JIT::ret()
{
    emitOpcode(OP_RET);
}

// X86 specific primitives
void JIT::move(Opcode op, Register reg, Offset offset)
{
    emitRex(reg, REX::NoX, offset.base);
    emitOpcode(op);
    //ASSERT(base != (Register)rsp, "TODO: SIB");
    if (!offset.offset && offset.base != (Register)rbp) {
        emitModRm(ModRM::None, reg, offset.base, offset.index);
    } else if (is8Bit(offset.offset)) {
        emitModRm(ModRM::Offset8Bit, reg, offset.base, offset.index);
        emitByte((int8_t)offset.offset);
    } else {
        emitModRm(ModRM::Offset32Bit, reg, offset.base, offset.index);
        emitLong(offset.offset);
    }
}

void  JIT::emitRex(Register r, Register x, Register b)
{
    emitByte(0x40 | 0x8 | (r >> 3) << 2 | (x >> 3) << 1 | (b >> 3));
}

void JIT::emitModRm(ModRM mod, uint8_t regOrOpcode, Register rm)
{
    emitModRm(mod, regOrOpcode, rm, SIB::NoIndex);
}

void JIT::emitModRm(ModRM mod, uint8_t regOrOpcode, Register rm, Register index)
{
    Register base = rm;
    if (mod != ModRM::Register && (rm == SIB::HasSib || index != SIB::NoIndex))
        base = SIB::HasSib;
    emitByte((uint8_t)mod << 6 | (regOrOpcode & 7) << 3 | (base & 7));
    if (mod != ModRM::Register && base == SIB::HasSib)
        emitSib(0, index, rm);
}

void JIT::emitSib(uint8_t scale, Register index, Register base)
{
    emitByte((scale & 3) << 6 | (index & 7) << 3 | (base & 7));
}

void JIT::emitOpcode(Opcode opcode)
{
    emitByte(opcode);
}

void JIT::emitOpcode(uint8_t opcode, uint8_t extra)
{
    emitByte(opcode + (extra & 7));
}
