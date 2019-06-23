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

Value UnificationScope::resolve(Type* resultType)
{
    finalize();
    return resultType->substitute(m_vm, m_substitutions);
}

Type* UnificationScope::infer(InstructionStream::Offset bytecodeOffset, TypeVar* var)
{
    finalize();
    auto it = m_substitutions.find(var->uid());
    if (it != m_substitutions.end())
        return it->second;
    std::stringstream message;
    message << "Unification failure: failed to infer type variable `" << *var << "`";
    LOG(ConstraintSolving, message.str());
    m_vm.typeError(bytecodeOffset, message.str());
    return m_vm.unitType;
}

void UnificationScope::finalize()
{
    if (m_finalized)
        return;
    m_finalized = true;
    solveConstraints();
}

void UnificationScope::solveConstraints()
{
    while (!m_constraints.empty()) {
        auto constraint = m_constraints.front();
        m_constraints.pop_front();
        unifies(constraint);
    }
}

void UnificationScope::unifies(const Constraint& constraint)
{
    Type* lhsType = constraint.lhs.type(m_vm)->substitute(m_vm, m_substitutions);
    Type* rhsType = constraint.rhs.asType()->substitute(m_vm, m_substitutions);

    LOG(ConstraintSolving, "Solving constraint: " << *lhsType << " U " << *rhsType << " @ " << m_vm.currentBlock->locationInfo(constraint.bytecodeOffset));

    if (rhsType->is<TypeVar>()) {
        TypeVar* var = rhsType->as<TypeVar>();
        if (var->inferred() && !var->isRigid()) {
            ASSERT(!var->isRigid(), "OOPS");
            bind(var, lhsType->substitute(m_vm, m_substitutions));
            return;
        }

        if (lhsType->is<TypeType>()) {
            ASSERT(constraint.lhs.isType(), "OOPS");
            TypeVar* var = rhsType->as<TypeVar>();
            if (!var->isRigid()) {
                lhsType = constraint.lhs.asType()->substitute(m_vm, m_substitutions);
                bind(var, lhsType);
            }
            return;
        }
    }

    if (*lhsType == *rhsType)
        return;

    if (rhsType->is<TypeVar>() && !rhsType->as<TypeVar>()->isRigid())
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

void pushUnificationScope(VM* vm)
{
    new UnificationScope(*vm);
}

void popUnificationScope(VM* vm)
{
    delete vm->unificationScope;
}
