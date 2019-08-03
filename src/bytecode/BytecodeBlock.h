#pragma once

#include "Cell.h"
#include "InstructionStream.h"
#include "SourceLocation.h"
#include "Value.h"
#include "expressions.h"
#include <iomanip>
#include <iostream>
#include <vector>

class Function;

class BytecodeBlock : public Cell {
    friend class BytecodeGenerator;

public:
    CELL(BytecodeBlock)

    void visit(const Visitor&) const;
    void dump(std::ostream&) const;

    const std::string& name() const { return m_name; }
    InstructionStream& instructions() { return m_instructions; }
    const InstructionStream& instructions() const { return m_instructions; }
    uint32_t numLocals() const { return m_numLocals; }
    Register environmentRegister() const { return m_environmentRegister; }
    InstructionStream::Offset codeStart() const { return m_codeStart; };

    const std::string& identifier(uint32_t) const;
    Value& constant(uint32_t) const;
    BytecodeBlock& functionBlock(uint32_t) const;
    uint32_t addFunctionBlock(BytecodeBlock*);
    Function* function(uint32_t) const;
    void setFunction(uint32_t, Function*);

    bool optimize(VM&) const;
    void* jitCode() const;

    void addLocation(const SourceLocation&);

    LocationInfoWithFile locationInfo(InstructionStream::Offset) const;

private:
    BytecodeBlock(std::string);

    void emitPrologue(const std::function<void()>&);
    void adjustOffsets();

    template<typename JumpType, typename Label>
    void recordJump(Label& label)
    {
        m_instructions.recordJump<JumpType>(m_prologueSize, label);
    }

    uint32_t m_numLocals { 0 };
    uint32_t m_prologueSize { 0 };
    InstructionStream::Offset m_codeStart { 0 };
    Register m_environmentRegister;
    std::string m_name;
    const char* m_filename { nullptr };
    InstructionStream m_instructions;
    mutable std::vector<Value> m_constants;
    std::vector<std::string> m_identifiers;
    std::vector<BytecodeBlock*> m_functionBlocks;
    std::vector<Function*> m_functions;
    std::vector<LocationInfo> m_locationInfos;

    // JIT
    mutable uint32_t m_hitCount = 0;
    mutable void* m_jitCode = nullptr;
};
