#pragma once

#include "InstructionStream.h"
#include "Value.h"
#include "expressions.h"
#include <iomanip>
#include <iostream>
#include <vector>

class BytecodeBlock {
    friend class BytecodeGenerator;

public:
    BytecodeBlock(std::string);

    void visit(std::function<void(Value)>) const;
    void dump(std::ostream&) const;

    const std::string& name() const { return m_name; }
    InstructionStream& instructions() { return m_instructions; }
    const InstructionStream& instructions() const { return m_instructions; }
    uint32_t numLocals() const { return m_numLocals; }

    const Identifier& identifier(uint32_t) const;
    Value constant(uint32_t) const;
    const BytecodeBlock& function(uint32_t) const;

    bool optimize(VM&, const Environment*) const;
    void* jitCode() const;

private:
    InstructionStream m_instructions;
    std::string m_name;
    uint32_t m_numLocals { 0 };
    std::vector<Value> m_constants;
    std::vector<Identifier> m_identifiers;
    std::vector<std::unique_ptr<BytecodeBlock>> m_functions;

    // JIT
    mutable uint32_t m_hitCount = 0;
    mutable void* m_jitCode = nullptr;
};
