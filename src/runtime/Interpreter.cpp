#include "Interpreter.h"

#include "Array.h"
#include "Function.h"
#include "Object.h"

Value& Interpreter::Stack::operator[](const Register& reg) const
{
    return m_stackAddress[reg.offset()];
}

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
    m_vm.stack.insert(m_vm.stack.begin(), args.begin(), args.end());

    // fill what would be return address
    m_vm.stack.insert(m_vm.stack.begin(), Value::crash());

    m_stop = false;
    while (!m_stop) {
        m_stop = true;
        dispatch();
    }

    m_vm.stack.erase(m_vm.stack.begin(), m_vm.stack.begin() + args.size() + 1);
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
    uint32_t cfrIndex = m_cfr.m_stackAddress - &vm().stack[0];
    Value result = function->call(vm(), args);
    ASSERT(vm().stack.size() == stackSize, "Inconsistent stack");
    m_cfr = Stack { &vm().stack[cfrIndex] };
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
    m_vm.stack.insert(m_vm.stack.begin(), m_block.numLocals(), Value::crash());
    m_cfr = Stack { &vm().stack[m_block.numLocals()] };
    DISPATCH();
}

OP(End)
{
    m_result = m_cfr[ip.dst];
    m_vm.stack.erase(m_vm.stack.begin(), m_vm.stack.begin() + m_block.numLocals());
}

OP(Move)
{
    m_cfr[ip.dst] = m_cfr[ip.src];
    DISPATCH();
}

OP(LoadConstant)
{
    m_cfr[ip.dst] = m_block.constant(ip.constantIndex);
    DISPATCH();
}

OP(GetLocal)
{

    m_cfr[ip.dst] = m_environment->get(m_block.identifier(ip.identifierIndex));
    DISPATCH();
}

OP(SetLocal)
{
    m_environment->set(m_block.identifier(ip.identifierIndex), m_cfr[ip.src]);
    DISPATCH();
}

OP(NewArray)
{
    m_cfr[ip.dst] = Value { Array::create(vm(), ip.initialSize) };
    DISPATCH();
}

OP(SetArrayIndex)
{
    Array* array = m_cfr[ip.src].asCell<Array>();
    Value value = m_cfr[ip.value];
    array->setIndex(ip.index, value);
    DISPATCH();
}

OP(GetArrayIndex)
{
    Array* array = m_cfr[ip.array].asCell<Array>();
    Value index = m_cfr[ip.index];
    Value result = array->getIndex(index);
    m_cfr[ip.dst] = result;
    DISPATCH();
}

OP(NewFunction)
{
    auto* function = Function::create(vm(), m_block.function(ip.functionIndex), m_environment);
    m_cfr[ip.dst] = Value { function };
    DISPATCH();
}

OP(Call)
{
    auto* function = m_cfr[ip.callee].asCell<Function>();
    std::vector<Value> args(ip.argc);
    uint32_t firstArgOffset = -ip.firstArg.offset();
    for (uint32_t i = 0; i < ip.argc; i++)
        args[i] = m_cfr[Register::forLocal(firstArgOffset + i)];
    auto result = call(function, args);
    m_cfr[ip.dst] = result;
    DISPATCH();
}

OP(NewObject)
{
    auto* object = Object::create(vm(), ip.inlineSize);
    m_cfr[ip.dst] = Value { object };
    DISPATCH();
}

OP(SetField)
{
    Object* object = m_cfr[ip.object].asCell<Object>();
    const Identifier& field = m_block.identifier(ip.fieldIndex);
    Value value = m_cfr[ip.value];
    object->set(field, value);
    DISPATCH();
}

OP(GetField)
{
    Object* object = m_cfr[ip.object].asCell<Object>();
    const Identifier& field = m_block.identifier(ip.fieldIndex);
    m_cfr[ip.dst] = object->get(field);
    DISPATCH();
}

OP(Jump)
{
    JUMP(ip.target);
}

OP(JumpIfFalse)
{
    Value condition = m_cfr[ip.condition];
    if (!condition.asBool())
        JUMP(ip.target);
    DISPATCH();
}

#undef JUMP
#undef DISPATCH
#undef OP
