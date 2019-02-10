#include "BytecodeBlock.h"

#include "JIT.h"
static uint32_t optimizationThreshold()
{
    static uint32_t threshold = std::getenv("JIT_THRESHOLD") ? atoi(std::getenv("JIT_THRESHOLD")) : 10;
    return threshold;
}

BytecodeBlock::BytecodeBlock(std::string name)
    : m_name(name)
{
}

const Identifier& BytecodeBlock::identifier(uint32_t index) const
{
    ASSERT(index < m_identifiers.size(), "Identifier out of bounds");
    return *std::next(m_identifiers.begin(), index);
}

Value BytecodeBlock::constant(uint32_t index) const
{
    ASSERT(index < m_constants.size(), "Constant out of bounds");
    return m_constants[index];
}

const BytecodeBlock& BytecodeBlock::function(uint32_t index) const
{
    ASSERT(index < m_functions.size(), "Function out of bounds");
    return *m_functions[index];
}

void BytecodeBlock::visit(std::function<void(Value)> visitor) const
{
    for (auto value : m_constants)
        visitor(value);

    for (auto& block : m_functions)
        block->visit(visitor);
}

void BytecodeBlock::dump(std::ostream& out) const
{
    out << "BytecodeBlock: " << m_name << ", locals: " << m_numLocals << std::endl;
    m_instructions.dump(out);
    out << std::endl << "    Constants: " << std::endl;
    for (unsigned i = 0; i < m_constants.size(); i++)
        out << std::setw(8) << i << ": " << m_constants[i] << std::endl;
    out << std::endl << "    Identifiers: " << std::endl;
    for (unsigned i = 0; i < m_identifiers.size(); i++)
        out << std::setw(8) << i << ": " << identifier(i) << std::endl;
    out << std::endl << "    Functions: " << std::endl;
    for (unsigned i = 0; i < m_functions.size(); i++)
        out << std::setw(8) << i << ": " << function(i).name() << std::endl;
    out << std::endl;
}

bool BytecodeBlock::optimize(VM& vm, const Environment* parentEnvironment) const
{
    if (std::getenv("NO_JIT"))
        return false;

    if (++m_hitCount > optimizationThreshold()) {
        m_jitCode = JIT::compile(vm, *this, parentEnvironment);
        return true;
    }
    return false;
}

void* BytecodeBlock::jitCode() const
{
    return m_jitCode;
}
