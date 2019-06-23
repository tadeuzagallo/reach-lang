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

    Type* type() const { return m_type; }
    void setParentEnvironment(Environment* parentEnvironment)
    {
        m_parentEnvironment = parentEnvironment;
    }

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
    Function(BytecodeBlock& block, Environment* parentEnvironment, Type* type)
        : m_type(type)
        , m_parentEnvironment(parentEnvironment)
        , m_block(&block)
    {
    }

    Function(NativeFunction nativeFunction, Type* type)
        : m_type(type)
        , m_nativeFunction(nativeFunction)
    {
    }

    Type* m_type { nullptr };
    Environment* m_parentEnvironment;
    BytecodeBlock* m_block { nullptr };
    NativeFunction m_nativeFunction { nullptr };
};
