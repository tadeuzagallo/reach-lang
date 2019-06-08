#include "Interpreter.h"

#include "Array.h"
#include "Function.h"
#include "Log.h"
#include "Object.h"
#include "Scope.h"
#include "Type.h"
#include "UnificationScope.h"

Value& Interpreter::Stack::operator[](const Register& reg) const
{
    return m_stackAddress[reg.offset()];
}

Interpreter::Interpreter(VM& vm, const BytecodeBlock& block, Environment* parentEnvironment)
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

Value Interpreter::run(std::vector<Value> args, const std::function<void()>& callback)
{
    m_vm.stack.insert(m_vm.stack.begin(), args.begin(), args.end());

    // fill what would be the return address
    m_vm.stack.insert(m_vm.stack.begin(), Value::crash());

    m_callback = callback;
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
    case Instruction::ID: { \
        const Instruction* instruction = reinterpret_cast<const Instruction*>(m_ip.get()); \
        LOG(InterpreterDispatch, "["  << m_ip.offset() << "] " << *instruction); \
        run##Instruction(*instruction); \
        break; \
    } \

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

Value Interpreter::reg(Register r) const
{
    return m_cfr[r];
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
    if (m_callback)
        m_callback();
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
    Type* type = m_cfr[ip.type].asType();
    auto* function = Function::create(vm(), m_block.function(ip.functionIndex), m_environment, type);
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
    const std::string& field = m_block.identifier(ip.fieldIndex);
    Value value = m_cfr[ip.value];
    object->set(field, value);
    DISPATCH();
}

OP(GetField)
{
    Object* object = m_cfr[ip.object].asCell<Object>();
    const std::string& field = m_block.identifier(ip.fieldIndex);
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

OP(IsEqual)
{
    m_cfr[ip.dst] = m_cfr[ip.lhs] == m_cfr[ip.rhs];
    DISPATCH();
}

OP(StoreGlobalConstant)
{
    m_vm.globalConstants[ip.constantIndex] = m_cfr[ip.value];
    DISPATCH();
}

OP(LoadGlobalConstant)
{
    m_cfr[ip.dst] = m_vm.globalConstants[ip.constantIndex];
    DISPATCH();
}

// Type checking

OP(GetType)
{

    m_cfr[ip.dst] = m_environment->get(m_block.identifier(ip.identifierIndex));
    DISPATCH();
}

OP(SetType)
{
    m_environment->set(m_block.identifier(ip.identifierIndex), m_cfr[ip.src]);
    DISPATCH();
}

OP(PushScope)
{
    m_vm.typingScope = new Scope(this);
    DISPATCH();
}

OP(PopScope)
{
    Scope* topScope = m_vm.typingScope;
    m_vm.typingScope = topScope->parent();
    delete topScope;
    DISPATCH();
}

OP(PushUnificationScope)
{
    new UnificationScope(m_vm);
    DISPATCH();
}

OP(PopUnificationScope)
{
    delete m_vm.unificationScope;
    DISPATCH();
}

OP(Unify)
{
    m_vm.unificationScope->unify(ip.lhs, ip.rhs, m_cfr[ip.lhs], m_cfr[ip.rhs]);
    DISPATCH();
}

OP(ResolveType)
{
    Type* type = m_cfr[ip.type].asCell<Type>();
    m_cfr[ip.dst] = m_vm.unificationScope->resolve(type);
    DISPATCH();
}

OP(CheckType)
{
    Value value = m_cfr[ip.type].asCell<Type>();
    bool result;
    switch (ip.expected) {
    case Type::Class::AnyValue:
        result = !value.isType();
        break;
    case Type::Class::AnyType:
        result = value.isType();
        break;
    default:
        goto specificType;
    }
    goto storeResult;

specificType: {
    Type* type = value.asType();
    switch (ip.expected) {
    case Type::Class::Type:
        result = type->is<TypeType>();
        break;
    case Type::Class::Bottom:
        result = type->is<TypeBottom>();
        break;
    case Type::Class::Name:
        result = type->is<TypeName>();
        break;
    case Type::Class::Function:
        result = type->is<TypeFunction>();
        break;
    case Type::Class::Array:
        result = type->is<TypeArray>();
        break;
    case Type::Class::Record:
        result = type->is<TypeRecord>();
        break;
    case Type::Class::Var:
        result = type->is<TypeVar>();
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

storeResult:
    m_cfr[ip.dst] = result;
    DISPATCH();
}

OP(CheckValue)
{
    Value value = m_cfr[ip.type];
    Type* type = value.type(m_vm);
    bool result;
    switch (ip.expected) {
    case Type::Class::AnyValue:
        ASSERT_NOT_REACHED();
        break;
    case Type::Class::AnyType:
        result = true;
        break;
    case Type::Class::Type:
        result = type->is<TypeType>();
        break;
    case Type::Class::Bottom:
        result = type->is<TypeBottom>();
        break;
    case Type::Class::Name:
        result = type->is<TypeName>();
        break;
    case Type::Class::Function:
        result = type->is<TypeFunction>();
        break;
    case Type::Class::Array:
        result = type->is<TypeArray>();
        break;
    case Type::Class::Record:
        result = type->is<TypeRecord>();
        break;
    case Type::Class::Var:
        result = type->is<TypeVar>();
        break;
    }
    m_cfr[ip.dst] = result;
    DISPATCH();
}

OP(TypeError)
{
    const std::string& message = m_block.identifier(ip.messageIndex);
    ASSERT(false, "%s", message.c_str());
}

OP(InferImplicitParameters)
{
    TypeFunction* function = m_cfr[ip.function].asCell<TypeFunction>();
    for (uint32_t i = 0; i < function->implicitParamCount(); i++)
        m_vm.unificationScope->infer(function->implicitParam(i));
    DISPATCH();
}

// Create new types

OP(NewVarType)
{
    const std::string& name = m_block.identifier(ip.nameIndex);
    m_cfr[ip.dst] = TypeVar::create(m_vm, name, ip.isInferred, true);
    DISPATCH();
}

OP(NewNameType)
{
    const std::string& name = m_block.identifier(ip.nameIndex);
    m_cfr[ip.dst] = TypeName::create(m_vm, name);
    DISPATCH();
}

OP(NewArrayType)
{
    Type* itemType = m_cfr[ip.itemType].asCell<Type>();
    m_cfr[ip.dst] = TypeArray::create(m_vm, itemType);
    DISPATCH();
}

OP(NewRecordType)
{
    Fields fields;
    uint32_t firstKeyOffset = -ip.firstKey.offset();
    uint32_t firstTypeOffset = -ip.firstType.offset();
    for (uint32_t i = 0; i < ip.fieldCount; i++) {
        Value keyIndex = m_cfr[Register::forLocal(firstKeyOffset + i)];
        const std::string& key = m_block.identifier(keyIndex.asNumber());
        Value type = m_cfr[Register::forLocal(firstTypeOffset + i)];
        fields.emplace(key, type.asCell<Type>());
    }
    m_cfr[ip.dst] = TypeRecord::create(m_vm, fields);
    DISPATCH();
}

OP(NewFunctionType)
{
    Types params;
    uint32_t firstParamOffset = -ip.firstParam.offset();
    for (uint32_t i = 0; i < ip.paramCount; i++) {
        Value param = m_cfr[Register::forLocal(firstParamOffset + i)];
        params.emplace_back(param.asCell<Type>());
    }
    Type* returnType = m_cfr[ip.returnType].asCell<Type>();
    m_cfr[ip.dst] = TypeFunction::create(m_vm, params, returnType);
    DISPATCH();
}

// New values from existing types

OP(NewType)
{
    ASSERT(false, "TODO");
    DISPATCH();
}

OP(NewValue)
{
    Value value = m_cfr[ip.type];
    ASSERT(value.isType(), "OOPS");
    m_cfr[ip.dst] = AbstractValue { value.asType() };
    DISPATCH();
}

OP(GetTypeForValue)
{
    Value value = m_cfr[ip.value];
    m_cfr[ip.dst] = value.type(m_vm)->instantiate(m_vm);
    DISPATCH();
}

#undef JUMP
#undef DISPATCH
#undef OP
