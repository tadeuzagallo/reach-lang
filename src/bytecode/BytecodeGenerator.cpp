#include "BytecodeGenerator.h"
#include "Instructions.h"

BytecodeGenerator::BytecodeGenerator(VM& vm, std::string name)
    : m_vm(vm)
{
    m_block = std::make_unique<BytecodeBlock>(name);
    emit<Enter>();
}

std::unique_ptr<BytecodeBlock> BytecodeGenerator::finalize(Register result)
{
    emit<End>(result);
    m_block->adjustOffsets();
    if (std::getenv("DUMP_BYTECODE"))
        m_block->dump(std::cout);
    return std::move(m_block);
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

void BytecodeGenerator::newArray(Register dst, unsigned size)
{
    emit<NewArray>(dst, size);
}

void BytecodeGenerator::setArrayIndex(Register src, unsigned offset, Register value)
{
    emit<SetArrayIndex>(src, offset, value);
}

void BytecodeGenerator::getArrayIndex(Register dst, Register array, Register index)
{
    emit<GetArrayIndex>(dst, array, index);
}

uint32_t BytecodeGenerator::newFunction(Register dst, std::unique_ptr<BytecodeBlock> block)
{
    uint32_t functionIndex = m_block->addFunctionBlock(std::move(block));
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

void BytecodeGenerator::newObject(Register dst, uint32_t inlineSize)
{
    emit<NewObject>(dst, inlineSize);
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

void BytecodeGenerator::resolveType(Register dst, Register type)
{
    emit<ResolveType>(dst, type);
}

void BytecodeGenerator::checkType(Register dst, Register type, Type::Class expected) {
    emit<CheckType>(dst, type, expected);
}

void BytecodeGenerator::checkValue(Register dst, Register type, Type::Class expected) {
    emit<CheckValue>(dst, type, expected);
}

void BytecodeGenerator::typeError(const char* message)
{
    uint32_t messageIndex = uniqueIdentifier(message);
    emit<TypeError>(messageIndex);
}

void BytecodeGenerator::inferImplicitParameters(Register function)
{
    emit<InferImplicitParameters>(function);
}

void BytecodeGenerator::endTypeChecking(Register type)
{
    emit<End>(type);
    m_block->m_codeStart = m_block->instructions().size();
    emit<Enter>();
}

// Types
void BytecodeGenerator::newVarType(Register result, const std::string& name, bool inferred)
{
    uint32_t nameIndex = uniqueIdentifier(name);
    emit<NewVarType>(result, nameIndex, inferred);
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

void BytecodeGenerator::newFunctionType(Register result, const std::vector<Register>& params, Register returnType)
{
    uint32_t paramCount = params.size();
    Register firstParam = paramCount ? params[0] : Register::forParameter(0);
    emit<NewFunctionType>(result, paramCount, firstParam, returnType);
}

void BytecodeGenerator::newValue(Register dst, Register type)
{
    emit<NewValue>(dst, type);
}

void BytecodeGenerator::newType(Register dst, Register type)
{
    emit<NewType>(dst, type);
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

void BytecodeGenerator::emit(Type::Class tc)
{
    m_block->instructions().emit(static_cast<uint8_t>(tc));
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
