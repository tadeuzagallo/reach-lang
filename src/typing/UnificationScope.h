#pragma once

#include "InstructionStream.h"
#include "Register.h"
#include "SourceLocation.h"
#include "Type.h"
#include <deque>

class Environment;
class Hole;

class UnificationScope {
public:
    UnificationScope(VM&);
    ~UnificationScope();

    void visit(const Visitor&) const;
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
    void unifies(InstructionStream::Offset, Value, Value);
    void bind(TypeVar*, Type*);
    void unificationOr(const std::function<void()>&, const std::function<void()>&);
    Type* eval(Value);

    bool m_finalized { false };
    bool m_unificationFailed { false };
    bool m_shouldThrowTypeError { true };
    uint32_t m_currentConstraint { 0 };
    VM& m_vm;
    UnificationScope* m_parentScope;
    Environment* m_environment { nullptr };
    std::vector<Constraint> m_constraints;
    Substitutions m_substitutions;
};
