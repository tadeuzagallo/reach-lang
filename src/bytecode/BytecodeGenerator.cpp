#include "BytecodeGenerator.h"
#include "Instructions.h"

BytecodeGenerator::BytecodeGenerator(VM& vm, std::string name)
    : m_vm(vm)
    , m_block(vm, name)
{
    emit<Enter>();
}

BytecodeGenerator::~BytecodeGenerator()
{
    m_block.destroy(m_vm);
}

BytecodeBlock* BytecodeGenerator::finalize(Register result)
{
    emit<End>(result);
    m_block->adjustOffsets();
    if (std::getenv("DUMP_BYTECODE"))
        m_block->dump(std::cout);
    return m_block.get();
}

Register BytecodeGenerator::newLocal()
{
    return Register::forLocal(++m_block->m_numLocals);
}

Label BytecodeGenerator::label()
{
    return Label { };
}

void BytecodeGenerator::branch(Register condition, const std::function<void()>& consequent, const std::function<void()>& alternate)
{
    Label alt = label();
    Label end = label();
    jumpIfFalse(condition, alt);
    consequent();
    jump(end);
    emit(alt);
    alternate();
    emit(end);
}

void BytecodeGenerator::emitLocation(const SourceLocation& location)
{
    m_block->addLocation(location);
}

void BytecodeGenerator::emitPrologue(const std::function<void()>& functor)
{
    m_block->emitPrologue([&] {
        m_block->instructions().emitPrologue(functor);
    });
}

void BytecodeGenerator::loadConstant(Register dst, Value value)
{
    m_block->m_constants.push_back(value);
    emit<LoadConstant>(dst, m_block->m_constants.size() - 1);
}

void BytecodeGenerator::loadConstantIndex(Register dst, uint32_t index)
{
    emit<LoadConstant>(dst, index);
}

uint32_t BytecodeGenerator::storeConstant(Register value)
{
    uint32_t constantIndex = m_block->m_constants.size();
    m_block->m_constants.push_back(Value::crash());
    emit<StoreConstant>(constantIndex, value);
    return constantIndex;
}

void BytecodeGenerator::getLocal(Register dst, const Identifier& ident)
{
    getLocal(dst, ident.name);
}

void BytecodeGenerator::getLocal(Register dst, const std::string& ident)
{
    uint32_t index = uniqueIdentifier(ident);
    emit<GetLocal>(dst, index);
}

void BytecodeGenerator::getLocalOrConstant(Register dst, const std::string& ident, Value constant)
{
    uint32_t identIndex = uniqueIdentifier(ident);
    uint32_t constantIndex = m_block->m_constants.size();
    m_block->m_constants.push_back(constant);
    emit<GetLocalOrConstant>(dst, identIndex, constantIndex);
}

void BytecodeGenerator::setLocal(const Identifier& ident, Register src)
{
    setLocal(ident.name, src);
}

void BytecodeGenerator::setLocal(const std::string& ident, Register src)
{
    uint32_t index = uniqueIdentifier(ident);
    emit<SetLocal>(index, src);
}

void BytecodeGenerator::call(Register dst, Register callee, const std::vector<Register>& args)
{
    unsigned argc = args.size();
    emit<Call>(dst, callee, argc, argc ? args[0] : Register::forParameter(0));
}

void BytecodeGenerator::newArray(Register dst, Register type, unsigned size)
{
    emit<NewArray>(dst, type, size);
}

void BytecodeGenerator::setArrayIndex(Register src, unsigned offset, Register value)
{
    emit<SetArrayIndex>(src, offset, value);
}

void BytecodeGenerator::getArrayIndex(Register dst, Register array, Register index)
{
    emit<GetArrayIndex>(dst, array, index);
}

void BytecodeGenerator::getArrayLength(Register dst, Register array)
{
    emit<GetArrayLength>(dst, array);
}

void BytecodeGenerator::newTuple(Register dst, Register type, unsigned size)
{
    emit<NewTuple>(dst, type, size);
}

void BytecodeGenerator::setTupleIndex(Register src, unsigned offset, Register value)
{
    emit<SetTupleIndex>(src, offset, value);
}

void BytecodeGenerator::getTupleIndex(Register dst, Register tuple, Register index)
{
    emit<GetTupleIndex>(dst, tuple, index);
}

uint32_t BytecodeGenerator::newFunction(Register dst, BytecodeBlock* block)
{
    uint32_t functionIndex = m_block->addFunctionBlock(block);
    emit<NewFunction>(dst, functionIndex);
    return functionIndex;
}

void BytecodeGenerator::newFunction(Register dst, uint32_t functionIndex)
{
    emit<NewFunction>(dst, functionIndex);
}

void BytecodeGenerator::move(Register from, Register to)
{
    if (from == to)
        return;
    emit<Move>(from, to);
}

void BytecodeGenerator::newObject(Register dst, Register type, uint32_t inlineSize)
{
    emit<NewObject>(dst, type, inlineSize);
}

void BytecodeGenerator::setField(Register object, const std::string& field, Register value)
{
    uint32_t fieldIndex = uniqueIdentifier(field);
    emit<SetField>(object, fieldIndex, value);
}

void BytecodeGenerator::getField(Register dst, Register object, const std::string& field)
{
    uint32_t fieldIndex = uniqueIdentifier(field);
    emit<GetField>(dst, object, fieldIndex);
}

void BytecodeGenerator::tryGetField(Register dst, Register object, const std::string& field, Label& target)
{
    m_block->recordJump<TryGetField>(target);
    uint32_t fieldIndex = uniqueIdentifier(field);
    emit<TryGetField>(dst, object, fieldIndex, 0);
}

void BytecodeGenerator::jump(Label& target)
{
    m_block->recordJump<Jump>(target);
    emit<Jump>(0);
}

void BytecodeGenerator::jumpIfFalse(Register condition, Label& target)
{
    m_block->recordJump<JumpIfFalse>(target);
    emit<JumpIfFalse>(condition, 0);
}

void BytecodeGenerator::isEqual(Register dst, Register lhs, Register rhs)
{
    emit<IsEqual>(dst, lhs, rhs);
}

void BytecodeGenerator::runtimeError(const SourceLocation& location, const char* message)
{
    emitLocation(location);
    uint32_t messageIndex = uniqueIdentifier(message);
    emit<RuntimeError>(messageIndex);
}

void BytecodeGenerator::isCell(Register dst, Register value, Cell::Kind kind)
{
    emit<IsCell>(dst, value, kind);
}

// Type checking

void BytecodeGenerator::pushScope()
{
    emit<PushScope>();
}

void BytecodeGenerator::popScope()
{
    emit<PopScope>();
}

void BytecodeGenerator::pushUnificationScope()
{
    emit<PushUnificationScope>();
}

void BytecodeGenerator::popUnificationScope()
{
    emit<PopUnificationScope>();
}

void BytecodeGenerator::unify(Register lhs, Register rhs)
{
    emit<Unify>(lhs, rhs);
}

void BytecodeGenerator::match(Register lhs, Register rhs)
{
    emit<Match>(lhs, rhs);
}

void BytecodeGenerator::resolveType(Register dst, Register type)
{
    emit<ResolveType>(dst, type);
}

void BytecodeGenerator::checkType(Register dst, Register type, Type::Class expected) {
    emit<CheckType>(dst, type, expected);
}

void BytecodeGenerator::checkTypeOf(Register dst, Register type, Type::Class expected) {
    emit<CheckTypeOf>(dst, type, expected);
}

void BytecodeGenerator::typeError(const SourceLocation& location, const char* message)
{
    emitLocation(location);
    uint32_t messageIndex = uniqueIdentifier(message);
    emit<TypeError>(messageIndex);
}

void BytecodeGenerator::inferImplicitParameters(Register function, const std::vector<Register>& parameters)
{
    ASSERT(parameters.size(), "OOPS");
    emit<InferImplicitParameters>(function, parameters.size(), parameters[0]);
}

void BytecodeGenerator::endTypeChecking(Register type)
{
    emit<End>(type);
    m_block->m_codeStart = m_block->instructions().size();
    emit<Enter>();
}

// Types
void BytecodeGenerator::newVarType(Register result, const std::string& name, bool inferred, bool rigid, Register bounds)
{
    uint32_t nameIndex = uniqueIdentifier(name);
    emit<NewVarType>(result, nameIndex, inferred, rigid, bounds);
}

void BytecodeGenerator::newNameType(Register result, const std::string& name)
{
    uint32_t nameIndex = uniqueIdentifier(name);
    emit<NewNameType>(result, nameIndex);
}

void BytecodeGenerator::newArrayType(Register result, Register itemType)
{
    emit<NewArrayType>(result, itemType);
}

void BytecodeGenerator::newTupleType(Register result, uint32_t itemCount)
{
    emit<NewTupleType>(result, itemCount);
}

void BytecodeGenerator::newRecordType(Register result, const std::vector<std::pair<std::string, Register>>& fields)
{
    // TODO: this is bad and I should feel bad
    uint32_t fieldCount = fields.size();
    Register firstKey = Register::forParameter(0);
    Register firstType = Register::forParameter(0);
    std::vector<Register> keys;
    for (const auto& field : fields) {
        keys.emplace_back(newLocal());
        uint32_t keyIndex = uniqueIdentifier(field.first);
        loadConstant(keys.back(), keyIndex);
    }
    if (fieldCount) {
        firstKey = keys[0];
        firstType = fields[0].second;
    }
    emit<NewRecordType>(result, fieldCount, firstKey, firstType);
}

void BytecodeGenerator::newFunctionType(Register result, const std::vector<Register>& params, Register returnType, uint32_t inferredParameters)
{
    uint32_t paramCount = params.size();
    Register firstParam = paramCount ? params[0] : Register::forParameter(0);
    emit<NewFunctionType>(result, paramCount, firstParam, returnType, inferredParameters);
}

void BytecodeGenerator::newUnionType(Register dst, Register lhs, Register rhs)
{
    emit<NewUnionType>(dst, lhs, rhs);
}

void BytecodeGenerator::newBindingType(Register dst, const std::string& name, Register type)
{
    uint32_t nameIndex = uniqueIdentifier(name);
    emit<NewBindingType>(dst, nameIndex, type);
}

void BytecodeGenerator::newCallHole(Register dst, Register callee, const std::vector<Register>& arguments)
{
    unsigned argumentCount = arguments.size();
    emit<NewCallHole>(dst, callee, argumentCount, argumentCount ? arguments[0] : Register::forParameter(0));
}

void BytecodeGenerator::newSubscriptHole(Register dst, Register target, Register index)
{
    emit<NewSubscriptHole>(dst, target, index);
}

void BytecodeGenerator::newMemberHole(Register dst, Register object, const std::string& field)
{
    uint32_t fieldIndex = uniqueIdentifier(field);
    emit<NewMemberHole>(dst, object, fieldIndex);
}


void BytecodeGenerator::newValue(Register dst, Register type)
{
    emit<NewValue>(dst, type);
}

void BytecodeGenerator::getTypeForValue(Register dst, Register value)
{
    emit<GetTypeForValue>(dst, value);
}

// Primitives

void BytecodeGenerator::emit(Instruction::ID instructionID)
{
    emit(static_cast<uint32_t>(instructionID));
}

void BytecodeGenerator::emit(Register reg)
{
    emit(static_cast<uint32_t>(reg.offset()));
}

void BytecodeGenerator::emit(Label& label)
{
    label.link(m_block->m_prologueSize, m_block->instructions().end());
}

void BytecodeGenerator::emit(uint32_t word)
{
    m_block->instructions().emit(word);
}

template<typename T>
std::enable_if_t<std::is_enum<T>::value, void> BytecodeGenerator::emit(T t)
{
    emit(static_cast<std::underlying_type_t<T>>(t));
}

uint32_t BytecodeGenerator::uniqueIdentifier(const std::string& ident)
{
    auto it = m_uniqueIdentifierMapping.find(ident);
    if (it != m_uniqueIdentifierMapping.end())
        return it->second;

    m_block->m_identifiers.push_back(ident);
    uint32_t index = m_block->m_identifiers.size() - 1;
    m_uniqueIdentifierMapping[ident] = index;
    return index;
}
