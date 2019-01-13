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

void BytecodeGenerator::loadConstant(Register dst, Value value)
{
    m_block->m_constants.push_back(value);
    emit<LoadConstant>(dst, m_block->m_constants.size() - 1);
}

void BytecodeGenerator::getLocal(Register dst, const Identifier& ident)
{
    uint32_t index = uniqueIdentifier(ident);
    emit<GetLocal>(dst, index);
}

void BytecodeGenerator::setLocal(const Identifier& ident, Register src)
{
    uint32_t index = uniqueIdentifier(ident);
    emit<SetLocal>(index, src);
}
void BytecodeGenerator::call(Register dst, Register callee, std::vector<Register>& args)
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

void BytecodeGenerator::newFunction(Register dst, std::unique_ptr<BytecodeBlock> block)
{
    m_block->m_functions.push_back(std::move(block));
    emit<NewFunction>(dst, m_block->m_functions.size() - 1);
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

void BytecodeGenerator::setField(Register object, const Identifier& field, Register value)
{
    uint32_t fieldIndex = uniqueIdentifier(field);
    emit<SetField>(object, fieldIndex, value);
}

void BytecodeGenerator::getField(Register dst, Register object, const Identifier& field)
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

uint32_t BytecodeGenerator::uniqueIdentifier(const Identifier& ident)
{
    auto it = m_uniqueIdentifierMapping.find(ident);
    if (it != m_uniqueIdentifierMapping.end())
        return it->second;

    m_block->m_identifiers.push_back(ident);
    uint32_t index = m_block->m_identifiers.size() - 1;
    m_uniqueIdentifierMapping[ident] = index;
    return index;
}
