#include "Function.h"
#include "Interpreter.h"

void Function::visit(std::function<void(Value)> visitor) const
{
    if (m_block)
        m_block->visit(visitor);
    if (m_parentEnvironment)
        m_parentEnvironment->visit(visitor);
}

Value Function::call(VM& vm, std::vector<Value> args)
{
    if (m_nativeFunction) {
        if (m_block)
            return reinterpret_cast<Value(*)(uint32_t, Value*, const Environment*)>(m_block->jitCode())(args.size(), &args[0], m_parentEnvironment);
        return m_nativeFunction(vm, args);
    }

    if (m_block->optimize(vm)) {
        m_nativeFunction = reinterpret_cast<Value(*)(VM&, std::vector<Value>)>(m_block->jitCode());
        return call(vm, args);
    }

    Interpreter interpreter { vm, *m_block, m_parentEnvironment };
    return interpreter.run(args);
}

Function* createFunction(VM& vm, const BytecodeBlock& block, const Environment* parentEnvironment)
{
    return Function::create(vm, block, parentEnvironment);
}
