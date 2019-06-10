#include "BytecodeBlock.h"

#include "Function.h"
#include "JIT.h"

static uint32_t optimizationThreshold()
{
    static uint32_t threshold = std::getenv("JIT_THRESHOLD") ? atoi(std::getenv("JIT_THRESHOLD")) : 10;
    return threshold;
}

BytecodeBlock::BytecodeBlock(std::string name)
    : m_name(name)
    , m_environmentRegister(Register::forLocal(++m_numLocals))
{
}

const std::string& BytecodeBlock::identifier(uint32_t index) const
{
    ASSERT(index < m_identifiers.size(), "Identifier out of bounds");
    return *std::next(m_identifiers.begin(), index);
}

Value BytecodeBlock::constant(uint32_t index) const
{
    ASSERT(index < m_constants.size(), "Constant out of bounds");
    return m_constants[index];
}

Value& BytecodeBlock::constant(uint32_t index)
{
    ASSERT(index < m_constants.size(), "Constant out of bounds");
    return m_constants[index];
}

BytecodeBlock& BytecodeBlock::functionBlock(uint32_t index) const
{
    ASSERT(index < m_functionBlocks.size(), "Function out of bounds");
    return *m_functionBlocks[index];
}

uint32_t BytecodeBlock::addFunctionBlock(std::unique_ptr<BytecodeBlock> block)
{
    uint32_t functionIndex = m_functionBlocks.size();
    m_functionBlocks.emplace_back(std::move(block));
    m_functions.emplace_back(nullptr);
    return functionIndex;
}

Function* BytecodeBlock::function(uint32_t index) const
{
    ASSERT(index < m_functions.size(), "Function out of bounds");
    return m_functions[index];
}

void BytecodeBlock::setFunction(uint32_t index, Function* function)
{
    ASSERT(index < m_functions.size(), "Trying to set function out of bounds");
    m_functions[index] = function;
}

void BytecodeBlock::visit(std::function<void(Value)> visitor) const
{
    for (auto value : m_constants)
        visitor(value);

    for (auto& block : m_functionBlocks)
        block->visit(visitor);

    for (auto* function : m_functions)
        if (function)
            function->visit(visitor);
}

void BytecodeBlock::dump(std::ostream& out) const
{
    out << "BytecodeBlock: " << m_name << ", locals: " << m_numLocals << std::endl;
    m_instructions.dump(out);
    out << std::endl << "    Constants: " << std::endl;
    for (unsigned i = 0; i < m_constants.size(); i++)
        out << std::setw(8) << i << ": " << Value::SafeDump(m_constants[i]) << std::endl;
    out << std::endl << "    Identifiers: " << std::endl;
    for (unsigned i = 0; i < m_identifiers.size(); i++)
        out << std::setw(8) << i << ": " << identifier(i) << std::endl;
    out << std::endl << "    Functions: " << std::endl;
    for (unsigned i = 0; i < m_functionBlocks.size(); i++)
        out << std::setw(8) << i << ": " << functionBlock(i).name() << std::endl;
    out << std::endl;
}

bool BytecodeBlock::optimize(VM& vm) const
{
    if (std::getenv("NO_JIT"))
        return false;

    if (++m_hitCount > optimizationThreshold()) {
        m_jitCode = JIT::compile(vm, *this);
        return true;
    }
    return false;
}

void* BytecodeBlock::jitCode() const
{
    return m_jitCode;
}

void BytecodeBlock::LocationInfoWithFile::dump(std::ostream& out) const
{
    out << filename << ":" << info.start.line << ":" << info.start.column;
}

auto BytecodeBlock::locationInfo(InstructionStream::Offset bytecodeOffset) const -> LocationInfoWithFile
{
    int low = 0;
    int high = m_locationInfos.size();
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (m_locationInfos[mid].bytecodeOffset <= bytecodeOffset)
            low = mid + 1;
        else
            high = mid;
    }

    if (!low)
        low = 1;

    const LocationInfo& info = m_locationInfos[low - 1];
    return { m_filename, info };
}

void BytecodeBlock::addLocation(const SourceLocation& location)
{
    ASSERT(!m_filename || location.file.name == m_filename, "OOPS");
    if (!m_filename)
        m_filename = location.file.name;
    if (m_locationInfos.size()) {
        const LocationInfo& info = m_locationInfos.back();
        if (info.start == location.start && info.end == location.end)
            return;
    }
    m_locationInfos.emplace_back(LocationInfo { static_cast<uint32_t>(m_instructions.size()), location.start, location.end });
}

void BytecodeBlock::emitPrologue(const std::function<void()>& functor)
{
    size_t initialSize = m_instructions.size();
    functor();
    size_t diff = m_instructions.size() - initialSize;
    m_prologueSize += diff;
    ASSERT(!m_codeStart, "OOPS");
}

void BytecodeBlock::adjustOffsets()
{
    for (LocationInfo& info : m_locationInfos)
        info.bytecodeOffset += m_prologueSize;
}
