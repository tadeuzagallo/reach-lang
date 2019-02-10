#pragma once

#include "BytecodeBlock.h"
#include "Cell.h"
#include "Environment.h"
#include "VM.h"
#include <vector>

using NativeFunction = std::function<Value(VM&, std::vector<Value>)>;

class Function : public Cell {
public:
    CELL(Function)

    void visit(std::function<void(Value)>) const override;

    void dump(std::ostream& out) const override
    {
        if (m_block)
            out << "<function " << m_block->name() << ">";
        else
            out << "<native function>";
    }

    Value call(VM&, std::vector<Value>);

private:
    Function(const BytecodeBlock& block, const Environment* parentEnvironment)
        : m_parentEnvironment(parentEnvironment)
        , m_block(&block)
    {
    }

    Function(NativeFunction nativeFunction)
        : m_nativeFunction(nativeFunction)
    {
    }

    const Environment* m_parentEnvironment;
    const BytecodeBlock* m_block { nullptr };
    NativeFunction m_nativeFunction { nullptr };
};
