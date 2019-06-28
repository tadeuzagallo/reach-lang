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
    solveConstraints();
    m_finalized = true;
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

    // optimize out checks of the same type (e.g. String U String, Void U Void, Type U Type ...)
    if (lhsType == rhsType)
        return;

    LOG(ConstraintSolving, "Solving constraint: " << *lhsType << " U " << *rhsType << " @ " << m_vm.currentBlock->locationInfo(constraint.bytecodeOffset));

    if (rhsType->is<TypeVar>()) {
        TypeVar* var = rhsType->as<TypeVar>();
        if (var->inferred() && !var->isRigid()) {
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

    if (constraint.lhs.isType()) {
        lhsType = constraint.lhs.asType()->substitute(m_vm, m_substitutions);
        if (lhsType->is<TypeVar>()) {
            TypeVar* var = lhsType->as<TypeVar>();
            if (var->inferred() && !var->isRigid()) {
                bind(var, rhsType->substitute(m_vm, m_substitutions));
                return;
            }
        }
    }

    if (lhsType->typeClass() == rhsType->typeClass()) {
        switch (lhsType->typeClass())  {
        case Type::Class::Tuple: {
            TypeTuple* lhs = lhsType->as<TypeTuple>();
            TypeTuple* rhs = rhsType->as<TypeTuple>();
            Array* lhsItemsTypes = lhs->itemsTypes();
            Array* rhsItemsTypes = rhs->itemsTypes();
            uint32_t size = rhsItemsTypes->size();
            if (lhsItemsTypes->size() <  size)
                break;
            for (uint32_t i = 0; i < size; i++)
                unify(constraint.bytecodeOffset, AbstractValue { lhsItemsTypes->getIndex(i).asType() }, rhsItemsTypes->getIndex(i));
            return;
        }
        case Type::Class::Record: {
            TypeRecord* lhs = lhsType->as<TypeRecord>();
            TypeRecord* rhs = rhsType->as<TypeRecord>();
            if (lhs->fields()->size() < rhs->fields()->size())
                break;
            bool failed = false;
            for (const auto& pair : *rhs->fields()) {
                auto lhsField = lhs->field(pair.first);
                // TODO: better error message
                if (!lhsField) {
                    failed = true;
                    break;
                }
                unify(constraint.bytecodeOffset, AbstractValue { lhsField }, pair.second);
            }
            if (!failed)
                return;
            break;
        }
        case Type::Class::Array: {
            TypeArray* lhs = lhsType->as<TypeArray>();
            TypeArray* rhs = rhsType->as<TypeArray>();
            unify(constraint.bytecodeOffset, AbstractValue { lhs->itemType().asType()}, rhs->itemType());
            return;
        }
        case Type::Class::Function: {
            TypeFunction* lhs = lhsType->as<TypeFunction>();
            TypeFunction* rhs = rhsType->as<TypeFunction>();
            if (lhs->params()->size() != rhs->params()->size())
                break;
            for (uint32_t i = 0; i < lhs->params()->size(); i++)
                unify(constraint.bytecodeOffset, rhs->param(i), lhs->param(i));
            unify(constraint.bytecodeOffset, AbstractValue { lhs->returnType().asType() }, rhs->returnType());
            return;
        }
        default:
            break;
        }
    }

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
