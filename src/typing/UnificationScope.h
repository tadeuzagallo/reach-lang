#pragma once

#include "Register.h"
#include "SourceLocation.h"
#include "Type.h"
#include <deque>

class UnificationScope {
public:
    UnificationScope(VM&);
    ~UnificationScope();

    void unify(/*const SourceLocation&,*/ Register, Register, Value, Value);
    Value resolve(Value);
    void infer(/*const SourceLocation&,*/ Value);

private:
    struct Constraint {
        /*SourceLocation location;*/
        Register lhsRegister;
        Register rhsRegister;
        Value lhs;
        Value rhs;
    };

    struct InferredType {
        /*SourceLocation location;*/
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
