#include "Function.h"
#include "Interpreter.h"

void Function::visit(const Visitor& visitor) const
{
    Typed::visit(visitor);
    visitor.visit(m_block);
    visitor.visit(m_parentEnvironment);
}

Value Function::call(VM& vm, std::vector<Value> args)
{
    if (m_nativeFunction) {
        if (m_block)
            return reinterpret_cast<Value(*)(uint32_t, Value*, Environment*)>(m_block->jitCode())(args.size(), &args[0], m_parentEnvironment);
        return m_nativeFunction(vm, args);
    }

    if (m_block->optimize(vm)) {
        m_nativeFunction = reinterpret_cast<Value(*)(VM&, std::vector<Value>)>(m_block->jitCode());
        return call(vm, args);
    }

    return Interpreter::run(vm, *m_block, m_parentEnvironment, args);
}
