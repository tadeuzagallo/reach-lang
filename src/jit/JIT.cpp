#include "JIT.h"

#include "Array.h"
#include "BytecodeBlock.h"
#include "Environment.h"
#include "Function.h"
#include "Object.h"
#include "Scope.h"
#include "Tuple.h"
#include "UnificationScope.h"
#include "Value.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#ifdef OFFSETOF
#undef OFFSETOF
#endif

#define OFFSETOF(__obj, __field) \
    static_cast<int32_t>(((uintptr_t)&(((__obj*)0xbbadbeef)->__field)) - 0xbbadbeefll)

static int32_t offset(JIT::VirtualRegister r)
{
    return r.offset() * 8;
}

struct JIT::Offset {
    int32_t offset;
    Register base;
    Register index = defaultIndex;
};

struct JIT::Label {
    Label(const Label&) = delete;
    Label& operator=(const Label&) = delete;

    bool linked { false };
    uint32_t offset { 0 };
    std::vector<uint32_t> references;
};

#include "X64Primitives.cpp.inl"

VM* JIT::vm()
{
    return &m_vm;
}

void* JIT::compile(VM& vm, const BytecodeBlock& block)
{
    JIT jit { vm, block };
    return jit.compile();
}

JIT::JIT(VM& vm, const BytecodeBlock& block)
    : m_vm(vm)
    , m_block(block)
{
}

void* JIT::compile()
{
#define CASE(Instruction) \
    case Instruction::ID: \
        emit##Instruction(*reinterpret_cast<const Instruction*>(instruction.get())); \
        break;

    for (auto instruction = m_block.instructions().at(m_block.codeStart()); instruction != m_block.instructions().end(); ++instruction) {
        m_bytecodeOffset = instruction.offset();
        m_bytecodeOffsetMapping.emplace(m_bytecodeOffset, m_buffer.size());
        switch (instruction->id) {
            FOR_EACH_INSTRUCTION(CASE)
        }
    }

    for (const auto& pair : m_jumps) {
        uint32_t bytecodeOffset = pair.first;
        uint32_t bufferOffset = pair.second;
        uint32_t targetBufferOffset = m_bytecodeOffsetMapping[bytecodeOffset];
        int32_t target = static_cast<int32_t>(targetBufferOffset) - static_cast<int32_t>(bufferOffset);
        uint8_t* targetBytes = reinterpret_cast<uint8_t*>(&target);
        for (uint32_t i = 0; i < sizeof(targetBufferOffset); ++i)
            m_buffer[bufferOffset - 4 + i] = targetBytes[i];
    }

    void* result = mmap(nullptr, m_buffer.size(), PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
    ASSERT(result != MAP_FAILED, "Failed to allocate executable memory");
    memmove(result, &m_buffer[0], m_buffer.size());

    {
        int fd = open("/tmp/jit.o", O_WRONLY | O_CREAT);
        write(fd, &m_buffer[0], m_buffer.size());
        close(fd);
    }
    return result;

#undef CASE
}

JIT::Label JIT::label()
{
    return Label {};
}

#define OP(Instruction) \
    void JIT::emit##Instruction(const Instruction& ip)

OP(Enter)
{
    UNUSED(ip);
    prologue();
    move(Value::crash(), regT0);
    // skip environmentRegister register
    for (uint32_t i = 2; i <= m_block.numLocals(); i++)
        store(regT0, regCFR, -static_cast<int32_t>(i));
}

OP(End)
{
    load(ip.dst, regR0);
    epilogue();
}

OP(Move)
{
    load(ip.src, regT0);
    store(regT0, ip.dst);
}

OP(LoadConstant)
{
    move(m_block.constant(ip.constantIndex), regT0);
    store(regT0, ip.dst);
}

OP(StoreConstant)
{
    UNUSED(ip);
    ASSERT(false, "StoreConstant should only be used during type checking, and type checking shouldn't JIT");
}

OP(GetLocal)
{
    push(regT2);
    push(regT3);
    load(m_block.environmentRegister(), regA0);
    move(&m_block.identifier(ip.identifierIndex), regA1);
    lea(Offset { 0x0, regSP }, regA2);
    call(&Environment::get);
    pop(regT3);
    pop(regT2);
    store(regR0, ip.dst);
}

OP(SetLocal)
{
    load(m_block.environmentRegister(), regA0);
    move(&m_block.identifier(ip.identifierIndex), regA1);
    load(ip.src, regA2);
    call<Environment, void, const std::string&, Value>(&Environment::set);
}

OP(NewArray)
{
    move(vm(), regA0);
    load(ip.type, regA1);
    move(ip.initialSize, regA2);
    call<Array*, VM&, Type*, uint32_t>(&createArray);
    store(regR0, ip.dst);
}

OP(SetArrayIndex)
{
    load(ip.src, regA0);
    move(ip.index, regA1);
    load(ip.value, regA2);
    call(&Array::setIndex);
}

OP(GetArrayIndex)
{
    load(ip.array, regA0);
    load(ip.index, regA1);
    call<Array, Value, Value>(&Array::getIndex);
    store(regR0, ip.dst);
}

OP(GetArrayLength)
{
    UNUSED(ip);
    ASSERT(false, "GetArrayLength is only used during type checking");
}

OP(NewTuple)
{
    move(vm(), regA0);
    load(ip.type, regA1);
    move(ip.initialSize, regA2);
    call<Tuple*, VM&, Type*, uint32_t>(&createTuple);
    store(regR0, ip.dst);
}

OP(SetTupleIndex)
{
    load(ip.tuple, regA0);
    move(ip.index, regA1);
    load(ip.value, regA2);
    call(&Tuple::setIndex);
}

OP(GetTupleIndex)
{
    load(ip.tuple, regA0);
    load(ip.index, regA1);
    call(&Tuple::getIndex);
    store(regR0, ip.dst);
}

OP(NewFunction)
{
    move(m_block.function(ip.functionIndex), regA0);
    load(m_block.environmentRegister(), regA1);
    call<Function, void, Environment*>(&Function::setParentEnvironment);

    move(m_block.function(ip.functionIndex), regT0);
    store(regT0, ip.dst);
}

OP(Call)
{
    move(vm(), regA0);
    load(ip.callee, regA1);
    move(ip.argc, regA2);
    lea(ip.firstArg, regA3);
    call<Value, VM&, Function*, uint32_t, Value*>(&JIT::trampoline);
    store(regR0, ip.dst);
}

OP(NewObject)
{
    move(vm(), regA0);
    load(ip.type, regA1);
    move(ip.inlineSize, regA2);
    call<Object*, VM&, Type*, uint32_t>(createObject);
    store(regR0, ip.dst);
}

OP(SetField)
{
    load(ip.object, regA0);
    move(&m_block.identifier(ip.fieldIndex), regA1);
    load(ip.value, regA2);
    call(&Object::set);
}

OP(GetField)
{
    load(ip.object, regA0);
    move(&m_block.identifier(ip.fieldIndex), regA1);
    call(&Object::get);
    store(regR0, ip.dst);
}

OP(Jump)
{
    jump(ip.target);
}

OP(JumpIfFalse)
{
    load(ip.condition, regT0);
    compare(regT0, Value { false });
    jumpIfEqual(ip.target);
}

OP(IsEqual)
{
    load(ip.lhs, regT0);
    load(ip.rhs, regT1);
    compare(regT0, regT1);
    setEqual(regT0);
    bitOr(Value::TagTypeBool, regT0);
}

OP(NewVarType)
{
    move(vm(), regA0);
    move(&m_block.identifier(ip.nameIndex), regA1);
    move(ip.isInferred, regA2);
    move(ip.isRigid, regA3);
    call(createTypeVar);
    store(regR0, ip.dst);
}

OP(NewNameType)
{
    move(vm(), regA0);
    move(&m_block.identifier(ip.nameIndex), regA1);
    call(createTypeName);
    store(regR0, ip.dst);
}

OP(NewArrayType)
{
    move(vm(), regA0);
    load(ip.itemType, regA1);
    call(createTypeArray);
    store(regR0, ip.dst);
}

OP(NewTupleType)
{
    move(vm(), regA0);
    move(ip.itemCount, regA1);
    call(createTypeTuple);
    store(regR0, ip.dst);
}

OP(NewRecordType)
{
    move(vm(), regA0);
    move(&m_block, regA1);
    move(ip.fieldCount, regA2);
    lea(ip.firstKey, regA3);
    lea(ip.firstType, regA4);
    call(createTypeRecord);
    store(regR0, ip.dst);
}

OP(NewFunctionType)
{
    move(vm(), regA0);
    move(ip.paramCount, regA1);
    lea(ip.firstParam, regA2);
    load(ip.returnType, regA3);
    move(ip.inferredParameters, regA4);
    call(createTypeFunction);
    store(regR0, ip.dst);
}

OP(NewUnionType)
{
    move(vm(), regA0);
    load(ip.lhs, regA1);
    load(ip.rhs, regA2);
    call(createTypeUnion);
    store(regR0, ip.dst);
}

OP(NewBindingType)
{
    move(vm(), regA0);
    move(&m_block.identifier(ip.nameIndex), regA1);
    load(ip.type, regA2);
    call(createTypeUnion);
    store(regR0, ip.dst);
}

#define TYPE_OP(Instruction) \
    OP(Instruction) { \
        UNUSED(ip); \
        ASSERT(false, "JIT should not be doing type checking"); \
    }

TYPE_OP(PushScope)
TYPE_OP(PopScope)
TYPE_OP(PushUnificationScope)
TYPE_OP(PopUnificationScope)
TYPE_OP(Unify)
TYPE_OP(ResolveType)
TYPE_OP(CheckType)
TYPE_OP(CheckTypeOf)
TYPE_OP(TypeError)
TYPE_OP(InferImplicitParameters)
TYPE_OP(NewCallHole)
TYPE_OP(NewSubscriptHole)
TYPE_OP(NewMemberHole)
TYPE_OP(NewValue)
TYPE_OP(GetTypeForValue)

#undef TYPE_OP
#undef OP

Value JIT::trampoline(VM& vm, Function* function, uint32_t argc, Value* argv)
{
    std::vector<Value> args(argc);
    for (uint32_t i = 0; i < argc; ++i)
        args[i] = argv[-static_cast<int32_t>(i)];
    return function->call(vm, args);
}

// HELPERS

void JIT::prologue()
{
    push(regCFR);
    move(regSP, regCFR);

    push(regA0);
    push(regA1);

    // Create a new environment
    move(vm(), regA0);
    move(regA2, regA1);
    call<Environment*, VM&, Environment*>(createEnvironment);

    pop(regA1);
    pop(regA0);

    // Copy arguments into stack
    shiftl(3, regA0);
    sub(regA0, regSP);
    {
        Label start = label();
        Label end = label();
        emitLabel(start);
        compare(regA0, Value { nullptr });
        jumpIfEqual(end);
        move(Offset { -0x8, regA1, regA0 }, regT2);
        move(regT2, Offset { -0x8, regSP, regA0 });
        sub(8, regA0);
        jump(start);
        emitLabel(end);
    }

    push(regCFR);
    move(regSP, regCFR);

    sub(m_block.numLocals() * 8, regSP);
    bitAnd(~0xFLL, regSP);
    store(regR0, m_block.environmentRegister());
}

void JIT::epilogue()
{
    move(regCFR, regSP);
    pop(regCFR);
    move(regCFR, regSP);
    pop(regCFR);
    ret();
}

// load(src, dst)
void JIT::load(VirtualRegister src, Register dst)
{
    // mov src * 8(%cfr), %dst
    move(Offset { offset(src), regCFR }, dst);
}

void JIT::load(AddressTag, VirtualRegister src, Register dst)
{
    // lea src * 8(%cfr), %dst
    lea(Offset { offset(src), regCFR }, dst);
}

// lea(src, dst)
void JIT::lea(VirtualRegister src, Register dst)
{
    // mov src * 8(%cfr), %dst
    lea(Offset { offset(src), regCFR }, dst);
}

void JIT::move(const void* immediate, Register dst)
{
    // mov $imm, %dst
    move((uint64_t)immediate, dst);
}

void JIT::move(Value value, Register dst)
{
    // mov $imm, %dst
    move(value.m_bits, dst);
}

void JIT::store(Register src, VirtualRegister dst)
{
    // mov %src, dst * 8(%cfr)
    move(src, Offset { offset(dst), regCFR });
}

void JIT::store(Register src, Register base, int32_t offset)
{
    // mov %src, offset * 8(%base)
    move(src, Offset { offset * 8, base });
}


// calls
template<typename T, typename Out, typename... In>
void JIT::call(Out(T::*function)(In...))
{
    call(*(void**)&function);
}

template<typename T, typename Out, typename... In>
void JIT::call(Out(T::*function)(In...) const)
{
    call(*(void**)&function);
}

template<typename Out, typename... In>
void JIT::call(Out(*function)(In...))
{
    call(*(void**)&function);
}


template<typename Out, typename... In>
void JIT::call(Out(*function)(In&&...))
{
    call(*(void**)&function);
}

void JIT::emitByte(uint8_t byte)
{
    m_buffer.push_back(byte);
}

void JIT::emitLong(uint32_t l)
{
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&l);
    for (uint32_t i = 0; i < 4; ++i)
        emitByte(bytes[i]);
}

void JIT::emitQuad(uint64_t quad)
{
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&quad);
    for (uint32_t i = 0; i < sizeof(quad); ++i)
        emitByte(bytes[i]);
}

void JIT::emitJumpTarget(int32_t target)
{
    emitLong(0);
    m_jumps.emplace_back(std::make_pair(static_cast<int32_t>(m_bytecodeOffset) + target, m_buffer.size()));
}

void JIT::emitJumpTarget(Label& label)
{
    if (label.linked)
        emitLong(static_cast<int32_t>(label.offset) - static_cast<int32_t>(m_buffer.size() + 4));
    else {
        emitLong(0);
        label.references.push_back(m_buffer.size());
    }
}

void JIT::emitLabel(Label& label)
{
    ASSERT(!label.linked, "Cannot emit the same label more than once");
    label.linked = true;
    label.offset = m_buffer.size();
    for (uint32_t offset : label.references) {
        int32_t target = static_cast<int32_t>(label.offset) - static_cast<int32_t>(offset);
        uint8_t* targetBytes = reinterpret_cast<uint8_t*>(&target);
        for (uint32_t i = 0; i < 4; ++i)
            m_buffer[offset - 4 + i] = targetBytes[i];
    }
    label.references.clear();
}
