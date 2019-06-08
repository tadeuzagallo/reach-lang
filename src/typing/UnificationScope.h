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
    Value resolve(Value);
    void infer(InstructionStream::Offset, Value);

private:
    struct Constraint {
        InstructionStream::Offset bytecodeOffset;
        Value lhs;
        Value rhs;
    };

    struct InferredType {
        InstructionStream::Offset bytecodeOffset;
        TypeVar* var;
    };

    void finalize();
    void solveConstraints();
    void checkInferredVariables();
    void unifies(const Constraint&);
    void bind(TypeVar*, Type*);

    bool m_finalized { false };
    VM& m_vm;
    UnificationScope* m_parentScope;
    std::deque<Constraint> m_constraints;
    std::deque<InferredType> m_inferredTypes;
    Substitutions m_substitutions;
};
