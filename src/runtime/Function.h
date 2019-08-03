#pragma once

#include "BytecodeBlock.h"
#include "Environment.h"
#include "Typed.h"
#include "VM.h"
#include <vector>

using NativeFunction = std::function<Value(VM&, std::vector<Value>)>;

class Function : public Typed {
public:
    CELL(Function)

    void setParentEnvironment(Environment* parentEnvironment)
    {
        m_parentEnvironment = parentEnvironment;
    }

    std::string name() const
    {
        if (m_block)
            return m_block->name();
        else
            return "anonymous";
    }

    void dump(std::ostream& out) const override
    {
        if (m_block)
            out << "<function " << name() << ">";
        else
            out << "<native function>";
    }

    Value call(VM&, std::vector<Value>);

protected:
    void visit(const Visitor&) const override;

private:
    Function(BytecodeBlock& block, Environment* parentEnvironment, Type* type)
        : Typed(type)
        , m_parentEnvironment(parentEnvironment)
        , m_block(&block)
    {
    }

    Function(NativeFunction nativeFunction, Type* type)
        : Typed(type)
        , m_nativeFunction(nativeFunction)
    {
    }

    Environment* m_parentEnvironment;
    BytecodeBlock* m_block { nullptr };
    NativeFunction m_nativeFunction { nullptr };
};
