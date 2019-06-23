#pragma once

#include "InstructionStream.h"
#include "Register.h"
#include "SourceLocation.h"
#include "Type.h"
#include <deque>

class UnificationScope {
public:
    UnificationScope(VM&);
    ~UnificationScope();

    void unify(InstructionStream::Offset, Value, Value);
    Value resolve(Type*);
    Type* infer(InstructionStream::Offset, TypeVar*);

private:
    struct Constraint {
        InstructionStream::Offset bytecodeOffset;
        Value lhs;
        Value rhs;
    };

    void finalize();
    void solveConstraints();
    void unifies(const Constraint&);
    void bind(TypeVar*, Type*);

    bool m_finalized { false };
    VM& m_vm;
    UnificationScope* m_parentScope;
    std::deque<Constraint> m_constraints;
    Substitutions m_substitutions;
};

extern "C" {
void pushUnificationScope(VM*);
void popUnificationScope(VM*);
}
