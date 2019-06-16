#include "Scope.h"

#include "Interpreter.h"

Scope::Scope(Interpreter* interpreter)
    : m_parent(interpreter->vm().typingScope)
    , m_interpreter(interpreter)
{
    m_interpreter->m_environment = Environment::create(m_interpreter->vm(), m_interpreter->m_environment);
}

Scope::~Scope()
{
    m_interpreter->m_environment = m_interpreter->m_environment->parent();
}

void pushScope(VM* vm)
{
    // TODO
}

void popScope(VM* vm)
{
    // TODO
}
