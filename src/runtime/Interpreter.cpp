#include "Interpreter.h"

#include "Array.h"
#include "Function.h"
#include "Hole.h"
#include "Log.h"
#include "Object.h"
#include "Scope.h"
#include "Tuple.h"
#include "Type.h"
#include "UnificationScope.h"
#include <sstream>

Value Interpreter::check(VM& vm, BytecodeBlock& block, Environment* parentEnvironment)
{
    LOG(InterpreterDispatch, "Checking " << block.name() << " @ " << block.locationInfo(0));
    Interpreter interpreter { vm, block, 0, parentEnvironment };
    interpreter.m_mode = Mode::Check;
    Value result = interpreter.run();
    LOG(InterpreterDispatch, "Done checking " << block.name() << ": " << result << " @ " << block.locationInfo(0));
    return result;
}

Value Interpreter::run(VM& vm, BytecodeBlock& block, Environment* parentEnvironment, const Values& args, const Callback& callback)
{
    LOG(InterpreterDispatch, "Running " << block.name() << " @ " << block.locationInfo(0));
    Interpreter interpreter { vm, block, block.codeStart(), parentEnvironment };
    Value result = interpreter.run(args, callback);
    LOG(InterpreterDispatch, "Done running " << block.name() << ": " << result << " @ " << block.locationInfo(0));
    return result;
}

Value& Interpreter::Stack::operator[](const Register& reg) const
{
    return m_stackAddress[reg.offset()];
}

Interpreter::Interpreter(VM& vm, BytecodeBlock& block, InstructionStream::Offset bytecodeOffset, Environment* parentEnvironment)
    : m_vm(vm)
    , m_block(block)
    , m_ip(m_block.instructions().at(bytecodeOffset))
    , m_result(Value::crash())
{
    m_lastBlock = vm.currentBlock;
    m_vm.currentBlock = &block;
    m_environment = Environment::create(vm, parentEnvironment ?: vm.globalEnvironment);
    m_lastInterpreter = m_vm.currentInterpreter;
    m_vm.currentInterpreter = this;
}

Interpreter::~Interpreter()
{
    m_vm.currentBlock = m_lastBlock;
    m_vm.currentInterpreter = m_lastInterpreter;
}

void Interpreter::visit(const Visitor& visitor) const
{
    visitor.visit(&m_block);
    visitor.visit(m_environment);
    visitor.visit(m_lastInterpreter);
    if (m_lastInterpreter)
        m_lastInterpreter->visit(visitor);
}

Value Interpreter::run(const Values& args, const Callback& callback)
{
    size_t initialStackSize = m_vm.stack.size();
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
    ASSERT(m_vm.stack.size() == initialStackSize, "Inconsistent stack");
    return m_result;
}

void Interpreter::dispatch()
{
#define CASE(Instruction) \
    case Instruction::ID: { \
        const Instruction* instruction = reinterpret_cast<const Instruction*>(m_ip.get()); \
        LOG(InterpreterDispatch, m_block.name() << "#" << m_ip.offset() << ": " << *instruction << " @ " << m_block.locationInfo(m_ip.offset())); \
        run##Instruction(*instruction); \
        break; \
    } \

    switch (m_ip->id) {
        FOR_EACH_INSTRUCTION(CASE)
    }

#undef CASE
}

template<typename Functor>
Value Interpreter::preserveStack(const Functor& functor)
{
    size_t stackSize = vm().stack.size();
    uint32_t cfrIndex = m_cfr.m_stackAddress - &vm().stack[0];
    auto result = functor();
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
    UNUSED(ip);
    m_vm.stack.insert(m_vm.stack.begin(), m_block.numLocals(), Value::crash());
    m_cfr = Stack { &vm().stack[m_block.numLocals()] };
    DISPATCH();
}

OP(End)
{
    m_result = m_cfr[ip.dst];
    if (m_callback)
        m_callback(*this);
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

OP(StoreConstant)
{
    m_block.constant(ip.constantIndex) = m_cfr[ip.value];
    DISPATCH();
}

OP(GetLocal)
{
    bool success;
    const std::string& variable = m_block.identifier(ip.identifierIndex);
    m_cfr[ip.dst] = m_environment->get(variable, success);
    if (!success) {
        std::stringstream message;
        message << "Unknown variable: `" << variable << "`";
        m_vm.typeError(m_ip.offset(), message.str());
    }
    DISPATCH();
}

OP(GetLocalOrConstant)
{
    bool success;
    const std::string& variable = m_block.identifier(ip.identifierIndex);
    m_cfr[ip.dst] = m_environment->get(variable, success);
    if (!success || m_cfr[ip.dst].isAbstractValue())
        m_cfr[ip.dst] = m_block.constant(ip.constantIndex);
    DISPATCH();
}

OP(SetLocal)
{
    m_environment->set(m_block.identifier(ip.identifierIndex), m_cfr[ip.src]);
    DISPATCH();
}

OP(NewArray)
{
    Value typeValue = m_cfr[ip.type];
    auto* type = typeValue.isUnit() ? nullptr : typeValue.asType();
    m_cfr[ip.dst] = Value { Array::create(vm(), type, ip.initialSize) };
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

OP(GetArrayLength)
{
    Array* array = m_cfr[ip.array].asCell<Array>();
    m_cfr[ip.dst] = static_cast<uint32_t>(array->size());
    DISPATCH();
}

OP(NewTuple)
{
    Value typeValue = m_cfr[ip.type];
    auto* type = typeValue.isUnit() ? nullptr : typeValue.asType();
    m_cfr[ip.dst] = Value { Tuple::create(vm(), type, ip.initialSize) };
    DISPATCH();
}

OP(SetTupleIndex)
{
    Tuple* tuple = m_cfr[ip.tuple].asCell<Tuple>();
    Value value = m_cfr[ip.value];
    tuple->setIndex(ip.index, value);
    DISPATCH();
}

OP(GetTupleIndex)
{
    Tuple* tuple = m_cfr[ip.tuple].asCell<Tuple>();
    Value index = m_cfr[ip.index];
    Value result = tuple->getIndex(index);
    m_cfr[ip.dst] = result;
    DISPATCH();
}

OP(NewFunction)
{
    Function* function;

    if (m_mode == Mode::Check) {
        BytecodeBlock& functionBlock = m_block.functionBlock(ip.functionIndex);
        Value type = preserveStack([&] {
            return check(vm(), functionBlock, m_environment);
        });
        ASSERT(type.isType(), "OOPS");
        function = Function::create(vm(), functionBlock, m_environment, type.asType());
        m_block.setFunction(ip.functionIndex, function);
    } else {
        function = m_block.function(ip.functionIndex);
        function->setParentEnvironment(m_environment);
    }

    m_cfr[ip.dst] = Value { function };
    DISPATCH();
}

OP(Call)
{
    auto* function = m_cfr[ip.callee].asCell<Function>();
    Values args(ip.argc);
    uint32_t firstArgOffset = -ip.firstArg.offset();
    for (uint32_t i = 0; i < ip.argc; i++)
        args[i] = m_cfr[Register::forLocal(firstArgOffset + i)];
    auto result = preserveStack([&] {
        return function->call(vm(), args);
    });
    m_cfr[ip.dst] = result;
    DISPATCH();
}

OP(NewObject)
{
    Value typeValue = m_cfr[ip.type];
    auto* type = typeValue.isUnit() ? nullptr : typeValue.asType();
    auto* object = Object::create(vm(), type, ip.inlineSize);
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

OP(TryGetField)
{
    Object* object = m_cfr[ip.object].asCell<Object>();
    const std::string& field = m_block.identifier(ip.fieldIndex);
    auto value = object->tryGet(field);
    if (!value)
        JUMP(ip.target);
    m_cfr[ip.dst] = *value;
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

OP(RuntimeError)
{
    const std::string& message = m_block.identifier(ip.messageIndex);
    m_vm.runtimeError(m_ip.offset(), message);
    DISPATCH();
}

OP(IsCell)
{
    Value value = m_cfr[ip.value];
    m_cfr[ip.dst] = value.isCell() && value.asCell()->kind() == ip.kind;
    DISPATCH();
}

// Type checking

OP(PushScope)
{
    UNUSED(ip);
    m_vm.typingScope = new Scope(this);
    DISPATCH();
}

OP(PopScope)
{
    UNUSED(ip);
    Scope* topScope = m_vm.typingScope;
    m_vm.typingScope = topScope->parent();
    delete topScope;
    DISPATCH();
}

OP(PushUnificationScope)
{
    UNUSED(ip);
    new UnificationScope(m_vm);
    DISPATCH();
}

OP(PopUnificationScope)
{
    UNUSED(ip);
    // This might call a nested interpreter, which might resize the underlying stack storage
    preserveStack([&] {
        delete m_vm.unificationScope;
        return Value::crash();
    });

    if (!m_vm.unificationScope) {
        // We are done type checking!
        bool hasErrors = m_vm.reportTypeErrors();
        if (hasErrors)
            exit(1);
    }

    DISPATCH();
}

OP(Unify)
{
    m_vm.unificationScope->unify(m_ip.offset(), m_cfr[ip.lhs], m_cfr[ip.rhs]);
    DISPATCH();
}

OP(Match)
{
    m_vm.unificationScope->match(m_ip.offset(), m_cfr[ip.lhs], m_cfr[ip.rhs]);
    DISPATCH();
}

OP(ResolveType)
{
    Type* type = m_cfr[ip.type].asType();
    // This might call a nested interpreter, which might resize the underlying stack storage
    m_cfr[ip.dst] = preserveStack([&] {
        return m_vm.unificationScope->resolve(type);
    });
    DISPATCH();
}

OP(CheckType)
{
    Value value = m_cfr[ip.type];
    bool result;
    static_assert(static_cast<uint8_t>(Type::Class::AnyValue) == 0);
    static_assert(static_cast<uint8_t>(Type::Class::AnyType) == 1);
    static_assert(static_cast<uint8_t>(Type::Class::SpecificType) == 2);
    if (ip.expected < Type::Class::SpecificType)
        result = static_cast<bool>(ip.expected) == value.isType();
    else
        result = value.isType() && ip.expected == value.asType()->typeClass();
    m_cfr[ip.dst] = result;
    DISPATCH();
}

OP(CheckTypeOf)
{
    Value value = m_cfr[ip.type];
    Type* type = value.type(m_vm);
    ASSERT(ip.expected >= Type::Class::SpecificType, "OOPS");
    m_cfr[ip.dst] = type->typeClass() == ip.expected;
    DISPATCH();
}

OP(TypeError)
{
    const std::string& message = m_block.identifier(ip.messageIndex);
    m_vm.typeError(m_ip.offset(), message);
    DISPATCH();
}

OP(InferImplicitParameters)
{
    TypeFunction* function = m_cfr[ip.function].asCell<TypeFunction>();
    uint32_t firstParameterOffset = -ip.firstParameter.offset();
    for (uint32_t i = 0; i < ip.parameterCount; i++) {
        Type* type = function->implicitParam(i);
        ASSERT(type->is<TypeBinding>(), "OOPS");
        type = type->as<TypeBinding>()->type();
        ASSERT(type->is<TypeVar>(), "OOPS");
        Type* result = m_vm.unificationScope->infer(m_ip.offset(), type->as<TypeVar>());
        m_cfr[Register::forLocal(firstParameterOffset + i)] = result;
    }
    DISPATCH();
}

// Create new types

OP(NewVarType)
{
    const std::string& name = m_block.identifier(ip.nameIndex);
    TypeVar* var = TypeVar::create(m_vm, name, ip.isInferred, ip.isRigid);
    m_cfr[ip.dst] = var;
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
    Value itemType = m_cfr[ip.itemType];
    m_cfr[ip.dst] = TypeArray::create(m_vm, itemType.asType());
    DISPATCH();
}

OP(NewTupleType)
{
    m_cfr[ip.dst] = TypeTuple::create(m_vm, ip.itemCount);
    DISPATCH();
}

OP(NewRecordType)
{
    Value* keys = &m_cfr[Register::forLocal(-ip.firstKey.offset() + ip.fieldCount - 1)];
    Value* types = &m_cfr[Register::forLocal(-ip.firstType.offset() + ip.fieldCount - 1)];
    m_cfr[ip.dst] = TypeRecord::create(m_vm, m_block, ip.fieldCount, keys, types);
    DISPATCH();
}

OP(NewFunctionType)
{
    Value* params = &m_cfr[Register::forLocal(-ip.firstParam.offset() + ip.paramCount - 1)];
    Value returnType = m_cfr[ip.returnType];
    m_cfr[ip.dst] = TypeFunction::create(m_vm, ip.paramCount, params, returnType.asType(), ip.inferredParameters);
    DISPATCH();
}

OP(NewUnionType)
{
    auto* unionType = TypeUnion::create(m_vm, m_cfr[ip.lhs].asType(), m_cfr[ip.rhs].asType());
    m_cfr[ip.dst] = unionType;
    DISPATCH();
}

OP(NewBindingType)
{
    const std::string& name = m_block.identifier(ip.nameIndex);
    auto* bindingTyp = TypeBinding::create(m_vm, String::create(m_vm, name), m_cfr[ip.type].asType());
    m_cfr[ip.dst] = bindingTyp;
    DISPATCH();
}

OP(NewCallHole)
{
    Value callee = m_cfr[ip.callee];
    Values args(ip.argc);
    uint32_t firstArgOffset = -ip.firstArg.offset();
    for (uint32_t i = 0; i < ip.argc; i++)
        args[i] = m_cfr[Register::forLocal(firstArgOffset + i)];
    m_cfr[ip.dst] = HoleCall::create(m_vm, callee, Array::create(m_vm, nullptr, std::move(args)));
    DISPATCH();
}

OP(NewSubscriptHole)
{
    Value target = m_cfr[ip.target];
    Value index = m_cfr[ip.index];
    m_cfr[ip.dst] = HoleSubscript::create(m_vm, target, index);
    DISPATCH();
}

OP(NewMemberHole)
{
    Value object = m_cfr[ip.object];
    const std::string& field = m_block.identifier(ip.fieldIndex);
    m_cfr[ip.dst] = HoleMember::create(m_vm, object, String::create(m_vm, field));
    DISPATCH();
}

// New values from existing types
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
