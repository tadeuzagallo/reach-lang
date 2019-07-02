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
    LOG(ConstraintSolving, message.str() << " @ " << m_vm.currentBlock->locationInfo(bytecodeOffset));
    m_unificationFailed = true;
    if (m_shouldThrowTypeError)
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
    while (m_currentConstraint < m_constraints.size())
        unifies(m_constraints[m_currentConstraint++]);
}

void UnificationScope::unifies(InstructionStream::Offset bytecodeOffset, Value lhs, Value rhs)
{
    unifies(Constraint { bytecodeOffset, lhs, rhs });
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
        // isInferred(T)
        // !isRigid(T)
        // ---------------------------------------- T-Value-InferredVar
        // x <: T => [T/σ]
        TypeVar* var = rhsType->as<TypeVar>();
        if (var->inferred() && !var->isRigid()) {
            bind(var, lhsType);
            return;
        }

        // isType(σ)
        // !isRigid(T)
        // ---------------------------------------- T-Type-Var
        // σ <: T => [T/σ]
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
        // isType(σ) // implicit, constraint.rhs is always a type
        // isInferred(T)
        // !isRigid(T)
        // ---------------------------------------- T-InferredVar-Type
        // T <: σ => [T/σ]
        if (lhsType->is<TypeVar>()) {
            TypeVar* var = lhsType->as<TypeVar>();
            if (var->inferred() && !var->isRigid()) {
                bind(var, rhsType->substitute(m_vm, m_substitutions));
                return;
            }
        }

        // NOTE: There's no T-Var-Type.
        // T-Type-Var is meant for type type arguments being passed explicit to
        // functions, so it's converse does not make sense.
    }

    // ∀σ, σ <: ⊤
    if (rhsType->is<TypeTop>())
        return;

    // ∀σ, ⊥ <: σ
    if (lhsType->is<TypeBottom>())
        return;

    // NOTE: T-Union-L must be applied before T-Union-R
    // That means that:
    // S | T <: S | T | U => S <: S | T | U ∧ T <: S | T | U => ⊤
    // Otherwise:
    // S | T <: S | T | U => S | T <: S ∧ S | T <: T ∧ S | T< : U => ⊥
    if (lhsType->is<TypeUnion>()) {
        auto* unionType = lhsType->as<TypeUnion>();
        if (Type* collapsed = unionType->collapse(m_vm)) {
            // {x: S | T } <: σ
            // ---------------------------------------- T-UnionRecord-L
            // {x: S} | {x: T} <: σ
            unifies(constraint.bytecodeOffset, AbstractValue { collapsed }, constraint.rhs);
            return;

            // TODO: ∀σ, T | T <: σ => T <: σ
        }

        // S <: σ
        // T <: σ
        // ---------------------------------------- T-Union-L
        // ∀σ, S | T <: σ
        unifies(constraint.bytecodeOffset, AbstractValue { unionType->lhs().asType() }, constraint.rhs);
        unifies(constraint.bytecodeOffset, AbstractValue { unionType->rhs().asType() }, constraint.rhs);
        return;
    }

    if (rhsType->is<TypeUnion>()) {
        // σ <: S ∨ σ <: T
        // ---------------------------------------- T-Union-R
        // σ <: S | T
        auto* unionType = rhsType->as<TypeUnion>();
        unificationOr([&] {
            unifies(constraint.bytecodeOffset, constraint.lhs, unionType->lhs());
        }, [&] {
            unifies(constraint.bytecodeOffset, constraint.lhs, unionType->rhs());
        });
        return;
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
                unifies(constraint.bytecodeOffset, AbstractValue { lhsItemsTypes->getIndex(i).asType() }, rhsItemsTypes->getIndex(i));
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
                unifies(constraint.bytecodeOffset, AbstractValue { lhsField }, pair.second);
            }
            if (!failed)
                return;
            break;
        }
        case Type::Class::Array: {
            TypeArray* lhs = lhsType->as<TypeArray>();
            TypeArray* rhs = rhsType->as<TypeArray>();
            unifies(constraint.bytecodeOffset, AbstractValue { lhs->itemType().asType()}, rhs->itemType());
            return;
        }
        case Type::Class::Function: {
            TypeFunction* lhs = lhsType->as<TypeFunction>();
            TypeFunction* rhs = rhsType->as<TypeFunction>();
            if (lhs->params()->size() != rhs->params()->size())
                break;
            for (uint32_t i = 0; i < lhs->params()->size(); i++)
                unifies(constraint.bytecodeOffset, rhs->param(i), lhs->param(i));
            unifies(constraint.bytecodeOffset, AbstractValue { lhs->returnType().asType() }, rhs->returnType());
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
    LOG(ConstraintSolving, msg.str() << " @ " << m_vm.currentBlock->locationInfo(constraint.bytecodeOffset));
    m_unificationFailed = true;
    if (m_shouldThrowTypeError)
        m_vm.typeError(constraint.bytecodeOffset, msg.str());
}

void UnificationScope::bind(TypeVar* var, Type* type)
{
    LOG(ConstraintSolving, "Binding type `" << *var << "` to `" << *type << "`");
    m_substitutions.emplace(var->uid(), type);
}

void UnificationScope::unificationOr(const std::function<void()>& lhs, const std::function<void()>& rhs)
{
    bool oldUnificationFailed = m_unificationFailed;
    bool oldShouldThrowTypeError = m_shouldThrowTypeError;
    m_unificationFailed = false;
    m_shouldThrowTypeError = false;
    lhs();
    std::swap(m_unificationFailed, oldUnificationFailed);
    m_shouldThrowTypeError = oldShouldThrowTypeError;
    if (oldUnificationFailed)
        rhs();
    // TODO: wrap rhs for better error message
}
