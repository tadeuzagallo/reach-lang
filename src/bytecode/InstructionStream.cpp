#include "InstructionStream.h"

#include "Assert.h"
#include <iomanip>

const Instruction* InstructionStream::Ref::get() const
{
    return reinterpret_cast<const Instruction*>(&m_instructions.m_instructions[m_offset]);
}

const Instruction* InstructionStream::Ref::operator->() const
{
    return get();
}

const InstructionStream::Ref& InstructionStream::Ref::operator*() const
{
    return *this;
}

bool InstructionStream::Ref::operator!=(const InstructionStream::Ref& other) const
{
    return &m_instructions != &other.m_instructions || m_offset != other.m_offset;
}

InstructionStream::Ref InstructionStream::Ref::operator++()
{
    m_offset += get()->size();
    return *this;
}

InstructionStream::Ref& InstructionStream::Ref::operator+=(int32_t target)
{
    m_offset += target;
    return *this;
}

InstructionStream::Offset InstructionStream::Ref::offset() const
{
    return m_offset;
}

void InstructionStream::WritableRef::write(uint32_t value)
{
    m_instructions.m_instructions[m_instructionOffset + m_targetOffset] = value;
}

InstructionStream::Offset InstructionStream::WritableRef::offset() const
{
    return m_instructionOffset;
}

InstructionStream::WritableRef::WritableRef(InstructionStream& instructions, Offset instructionOffset, uint32_t targetOffset)
    : m_instructions(instructions)
    , m_instructionOffset(instructionOffset)
    , m_targetOffset(targetOffset)
{
}

void InstructionStream::Ref::dump(std::ostream& out) const
{
    out << "[" << std::setw(4) << m_offset << "] ";
    get()->dump(out);
}

InstructionStream::Ref::Ref(const InstructionStream& instructions, InstructionStream::Offset offset)
    : m_instructions(instructions)
      , m_offset(offset)
{
}

InstructionStream::Ref InstructionStream::at(Offset offset) const
{
    ASSERT(offset < m_instructions.size(), "out of bounds instruction access");
    return InstructionStream::Ref { *this, offset };
}

InstructionStream::Ref InstructionStream::begin() const
{
    return InstructionStream::Ref { *this, 0 };
}

InstructionStream::Ref InstructionStream::end() const
{
    return InstructionStream::Ref { *this, m_instructions.size() };
}

void InstructionStream::emit(uint32_t word)
{
    m_instructions.push_back(word);
}

void InstructionStream::dump(std::ostream& out) const
{
    for (Ref instruction : *this) {
        instruction.dump(out);
        out << "\n";
    }
}
