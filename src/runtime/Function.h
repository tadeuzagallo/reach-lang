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
        if (m_isNativeFunction)
            out << "<native function>";
        else
            out << "<function " << m_block->name() << ">";
    }

    Value call(VM&, std::vector<Value>);

private:
    Function(const BytecodeBlock& block, const Environment* parentEnvironment)
        : m_isNativeFunction(false)
        , m_block(&block)
        , m_parentEnvironment(parentEnvironment)
    {
    }

    Function(NativeFunction nativeFunction)
        : m_isNativeFunction(true)
        , m_nativeFunction(nativeFunction)
        , m_parentEnvironment(nullptr)
    {
    }

    bool m_isNativeFunction;
    union {
        const BytecodeBlock* m_block;
        NativeFunction m_nativeFunction;
    };
    const Environment* m_parentEnvironment;
};
