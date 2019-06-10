#include "UnificationScope.h"

#include "BytecodeBlock.h"
#include "Log.h"
#include "TypeChecker.h"
#include <sstream>

UnificationScope::UnificationScope(VM& vm)
    : m_vm(vm)
    , m_parentScope(vm.unificationScope)
{
    LOG(UnificationScope, "===> UnificationScope <===");
    m_vm.unificationScope = this;
}

UnificationScope::~UnificationScope()
{
    finalize();
    m_vm.unificationScope = m_parentScope;
    LOG(UnificationScope,  "===> ~UnificationScope <===");
}

void UnificationScope::unify(InstructionStream::Offset bytecodeOffset, Value lhs, Value rhs)
{
    ASSERT(!m_finalized, "UnificationScope already finalized");
    ASSERT(rhs.isType(), "OOPS"); // You can only unify with a type
    m_constraints.emplace_back(Constraint { bytecodeOffset, lhs, rhs });
}

Value UnificationScope::resolve(Value resultType)
{
    finalize();
    ASSERT(resultType.isType(), "OOPS");
    return resultType.asType()->substitute(m_vm, m_substitutions);
}

void UnificationScope::infer(InstructionStream::Offset bytecodeOffset, Value value)
{
    ASSERT(value.isType(), "OOPS");
    Type* type = value.asType();
    ASSERT(type->is<TypeVar>(), "OOPS");
    m_inferredTypes.emplace_back(InferredType { bytecodeOffset, type->as<TypeVar>() });
}

void UnificationScope::finalize()
{
    if (m_finalized)
        return;
    m_finalized = true;
    solveConstraints();
    checkInferredVariables();
}

void UnificationScope::solveConstraints()
{
    while (!m_constraints.empty()) {
        auto constraint = m_constraints.front();
        m_constraints.pop_front();
        unifies(constraint);
    }
}

void UnificationScope::checkInferredVariables()
{
    for (const InferredType& ib : m_inferredTypes) {
        auto it = m_substitutions.find(ib.var->uid());
        if (it != m_substitutions.end())
            continue;
        std::stringstream message;
        message << "Unification failure: failed to infer type variable `" << *ib.var << "`";
        LOG(ConstraintSolving, message.str());
        m_vm.typeError(ib.bytecodeOffset, message.str());
    }
}

void UnificationScope::unifies(const Constraint& constraint)
{
    Type* lhsType = constraint.lhs.type(m_vm);
    Type* rhsType = constraint.rhs.asType()->substitute(m_vm, m_substitutions);

    LOG(ConstraintSolving, "Solving constraint: " << *lhsType << " U " << *rhsType << " @ " << m_vm.currentBlock->locationInfo(constraint.bytecodeOffset));

    if (lhsType->is<TypeType>() && rhsType->is<TypeVar>()) {
        ASSERT(constraint.lhs.isType(), "OOPS");
        TypeVar* var = rhsType->as<TypeVar>();
        if (!var->isRigid()) {
            lhsType = constraint.lhs.asType()->substitute(m_vm, m_substitutions);
            bind(var, lhsType);
        }
        return;
    }

    if (*lhsType == *rhsType)
        return;

    if (rhsType->is<TypeVar>())
        rhsType = m_vm.typeType;

    std::stringstream msg;
    msg << "Unification failure: expected `" << *rhsType << "` but found `" << *lhsType << "`";
    LOG(ConstraintSolving, msg.str());
    m_vm.typeError(constraint.bytecodeOffset, msg.str());
}

void UnificationScope::bind(TypeVar* var, Type* type)
{
    LOG(ConstraintSolving, "Type " << *var << " to " << *type);
    m_substitutions.emplace(var->uid(), type);
}
