#include "Function.h"
#include "Interpreter.h"

void Function::visit(std::function<void(Value)> visitor) const
{
    if (m_parentEnvironment)
        m_parentEnvironment->visit(visitor);
    if (!m_isNativeFunction)
        m_block->visit(visitor);
}

Value Function::call(VM& vm, std::vector<Value> args)
{
    if (m_isNativeFunction) {
        return m_nativeFunction(vm, args);
    }

    Interpreter interpreter { vm, *m_block, m_parentEnvironment };
    return interpreter.run(args);
}
