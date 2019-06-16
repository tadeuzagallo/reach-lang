#pragma once

#include <stddef.h>

class Interpreter;
class VM;

class Scope {
public:
    Scope(Interpreter*);
    ~Scope();

    Scope* parent() const { return m_parent; }

private:
    Scope* m_parent;
    Interpreter* m_interpreter;
};

extern "C" {
void popScope(VM*);
void pushScope(VM*);
};
