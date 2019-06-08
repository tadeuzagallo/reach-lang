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

void BytecodeGenerator::loadConstant(Register dst, Value value)
{
    m_block->m_constants.push_back(value);
    emit<LoadConstant>(dst, m_block->m_constants.size() - 1);
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

void BytecodeGenerator::newFunction(Register dst, std::unique_ptr<BytecodeBlock> block, Register type)
{
    m_block->m_functions.push_back(std::move(block));
    emit<NewFunction>(dst, m_block->m_functions.size() - 1, type);
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
    m_block->instructions().recordJump<Jump>(target);
    emit<Jump>(0);
}

void BytecodeGenerator::jumpIfFalse(Register condition, Label& target)
{
    m_block->instructions().recordJump<JumpIfFalse>(target);
    emit<JumpIfFalse>(condition, 0);
}

void BytecodeGenerator::isEqual(Register dst, Register lhs, Register rhs)
{
    emit<IsEqual>(dst, lhs, rhs);
}

// Type checking

void BytecodeGenerator::getType(Register dst, const std::string& ident)
{
    uint32_t index = uniqueIdentifier(ident);
    emit<GetType>(dst, index);
}

void BytecodeGenerator::setType(const std::string& ident, Register src)
{
    uint32_t index = uniqueIdentifier(ident);
    emit<SetType>(index, src);
}

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
    label.link(m_block->instructions().end());
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
