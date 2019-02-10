#include "Interpreter.h"

#include "Array.h"
#include "Function.h"
#include "Object.h"

Interpreter::Interpreter(VM& vm, const BytecodeBlock& block, const Environment* parentEnvironment)
    : m_vm(vm)
    , m_block(block)
    , m_ip(m_block.instructions().at(0))
    , m_result(Value::crash())
{
    m_environment = Environment::create(vm, parentEnvironment);
}

Interpreter::~Interpreter()
{
}

Value Interpreter::run(std::vector<Value> args)
{
    for (auto arg : args)
        m_vm.stack.push_back(arg);

    m_stop = false;
    while (!m_stop) {
        m_stop = true;
        dispatch();
    }

    for (uint32_t i = args.size(); i--;)
        m_vm.stack.pop_back();
    return m_result;
}

void Interpreter::dispatch()
{
#define CASE(Instruction) \
    case Instruction::ID: \
        run##Instruction(*reinterpret_cast<const Instruction*>(m_ip.get())); \
        break; 

    switch (m_ip->id) {
        FOR_EACH_INSTRUCTION(CASE)
    }

#undef CASE
}

Value Interpreter::call(Function* function, std::vector<Value> args)
{
    size_t stackSize = vm().stack.size();
    uint32_t cfrIndex = m_cfr - &vm().stack[0];
    Value result = function->call(vm(), args);
    ASSERT(vm().stack.size() == stackSize, "Inconsistent stack");
    m_cfr = &vm().stack[cfrIndex];
    return result;
}

#define OP(Instruction) \
    void Interpreter::run##Instruction(const Instruction& ip)

#define DISPATCH() \
    do { \
        m_ip.operator++(); \
        m_stop = false; \
        return; \
    } while (false)

#define JUMP(__target) \
    do { \
        m_ip += __target; \
        m_stop = false; \
        return; \
    } while (false)

OP(Enter)
{
    uint32_t cfrIndex = vm().stack.size() - 1;
    for (size_t i = 0; i < m_block.numLocals(); i++)
        m_vm.stack.push_back(Value::crash());
    m_cfr = &vm().stack[cfrIndex];
    DISPATCH();
}

OP(End)
{
    m_result = m_cfr[ip.dst.offset()];
    for (size_t i = 0; i < m_block.numLocals(); i++)
        m_vm.stack.pop_back();
}

OP(Move)
{
    m_cfr[ip.dst.offset()] = m_cfr[ip.src.offset()];
    DISPATCH();
}

OP(LoadConstant)
{
    m_cfr[ip.dst.offset()] = m_block.constant(ip.constantIndex);
    DISPATCH();
}

OP(GetLocal)
{

    m_cfr[ip.dst.offset()] = m_environment->get(m_block.identifier(ip.identifierIndex));
    DISPATCH();
}

OP(SetLocal)
{
    m_environment->set(m_block.identifier(ip.identifierIndex), m_cfr[ip.src.offset()]);
    DISPATCH();
}

OP(NewArray)
{
    m_cfr[ip.dst.offset()] = Value { Array::create(vm(), ip.initialSize) };
    DISPATCH();
}

OP(SetArrayIndex)
{
    Array* array = m_cfr[ip.src.offset()].asCell<Array>();
    Value value = m_cfr[ip.value.offset()];
    array->setIndex(ip.index, value);
    DISPATCH();
}

OP(GetArrayIndex)
{
    Array* array = m_cfr[ip.array.offset()].asCell<Array>();
    Value index = m_cfr[ip.index.offset()];
    Value result = array->getIndex(index);
    m_cfr[ip.dst.offset()] = result;
    DISPATCH();
}

OP(NewFunction)
{
    auto* function = Function::create(vm(), m_block.function(ip.functionIndex), m_environment);
    m_cfr[ip.dst.offset()] = Value { function };
    DISPATCH();
}

OP(Call)
{
    auto* function = m_cfr[ip.callee.offset()].asCell<Function>();
    std::vector<Value> args(ip.argc);
    uint32_t lastArgOffset = ip.firstArg.offset() + ip.argc;
    for (uint32_t i = 0; i < ip.argc; i++)
        args[i] = m_cfr[lastArgOffset - i - 1];
    auto result = call(function, args);
    m_cfr[ip.dst.offset()] = result;
    DISPATCH();
}

OP(NewObject)
{
    auto* object = Object::create(vm(), ip.inlineSize);
    m_cfr[ip.dst.offset()] = Value { object };
    DISPATCH();
}

OP(SetField)
{
    Object* object = m_cfr[ip.object.offset()].asCell<Object>();
    const Identifier& field = m_block.identifier(ip.fieldIndex);
    Value value = m_cfr[ip.value.offset()];
    object->set(field, value);
    DISPATCH();
}

OP(GetField)
{
    Object* object = m_cfr[ip.object.offset()].asCell<Object>();
    const Identifier& field = m_block.identifier(ip.fieldIndex);
    m_cfr[ip.dst.offset()] = object->get(field);
    DISPATCH();
}

OP(Jump)
{
    JUMP(ip.target);
}

OP(JumpIfFalse)
{
    Value condition = m_cfr[ip.condition.offset()];
    if (!condition.asBool())
        JUMP(ip.target);
    DISPATCH();
}

#undef JUMP
#undef DISPATCH
#undef OP
