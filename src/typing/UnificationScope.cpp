#include "UnificationScope.h"

#include "BytecodeBlock.h"
#include "BytecodeGenerator.h"
#include "Hole.h"
#include "HoleCodegen.h"
#include "Interpreter.h"
#include "Log.h"
#include "PartialEvaluator.h"
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

void UnificationScope::visit(const Visitor& visitor) const
{
    visitor(m_environment);
    for (const auto& constraint : m_constraints) {
        visitor(constraint.lhs);
        visitor(constraint.rhs);
    }
    for (const auto& pair : m_substitutions)
        visitor(pair.second);
    if (m_parentScope)
        m_parentScope->visit(visitor);
}

void UnificationScope::unify(InstructionStream::Offset bytecodeOffset, Value lhs, Value rhs)
{
    ASSERT(!m_finalized, "UnificationScope already finalized");
    ASSERT(rhs.isType(), "OOPS"); // You can only unify with a type
    m_constraints.emplace_back(Constraint { bytecodeOffset, lhs, rhs, Constraint::Type::Subtyping });
}

void UnificationScope::match(InstructionStream::Offset bytecodeOffset, Value lhs, Value rhs)
{
    ASSERT(!m_finalized, "UnificationScope already finalized");
    ASSERT(rhs.isType(), "OOPS"); // You can only unify with a type
    m_constraints.emplace_back(Constraint { bytecodeOffset, lhs, rhs, Constraint::Type::PatternMatching });
}

Value UnificationScope::resolve(Type* resultType)
{
    finalize();
    if (resultType->is<Hole>()) {
        // TODO: we should call partiallyEvaluate instead of eval if resolveType has unresolved holes
        // return partiallyEvaluate(Value { resultType }, m_vm, m_environment);
        return eval(resultType->as<Hole>());
    } else
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
    m_unificationFailed = true;
    if (m_shouldThrowTypeError) {
        LOG(ConstraintSolving, message.str() << " @ " << m_vm.currentBlock->locationInfo(bytecodeOffset));
        m_vm.typeError(bytecodeOffset, message.str());
    }
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
    ASSERT(m_finalized && !m_environment, "OOPS");
    ASSERT(m_vm.currentInterpreter, "OOPS");
    m_environment = Environment::create(m_vm, m_vm.currentInterpreter->environment());
    while (m_currentConstraint < m_constraints.size())
        unifies(m_constraints[m_currentConstraint++]);
}

void UnificationScope::unifies(const Constraint& constraint)
{
    switch (constraint.type) {
    case Constraint::Type::Subtyping:
        unifies(constraint.bytecodeOffset, constraint.lhs, constraint.rhs);
        break;
    case Constraint::Type::PatternMatching:
        matches(constraint.bytecodeOffset, constraint.lhs, constraint.rhs);
        break;
    }
}

void UnificationScope::unifies(InstructionStream::Offset bytecodeOffset, Value lhs, Value rhs)
{
    Type* lhsType = lhs.type(m_vm)->substitute(m_vm, m_substitutions);
    Type* rhsType = rhs.asType()->substitute(m_vm, m_substitutions);

    // Optimize out checks of the same type (e.g. String U String, Void U Void, Type U Type ...)
    //
    // ---------------------------------------- T-Ident
    // σ <: σ
    if (lhsType == rhsType)
        return;

    // Γ ⊢ σ <: τ
    // ---------------------------------------- T-Binding-L
    // Γ ⊢ x : σ <: τ
    if (lhsType->is<TypeBinding>()) {
        TypeBinding* binding = lhsType->as<TypeBinding>();
        unifies(bytecodeOffset, AbstractValue { binding->type() }, rhs);
        return;
    }

    // Γ, x: σ ⊢ σ <: τ
    // ---------------------------------------- T-Binding-R
    // Γ ⊢ σ <: x : τ
    if (rhsType->is<TypeBinding>()) {
        TypeBinding* binding = rhsType->as<TypeBinding>();
        m_environment->set(binding->name()->str(), lhs);
        unifies(bytecodeOffset, lhs, binding->type());
        return;
    }

    LOG(ConstraintSolving, "Solving constraint: " << *lhsType << " <: " << *rhsType << " @ " << m_vm.currentBlock->locationInfo(bytecodeOffset));

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
            ASSERT(lhs.isType(), "OOPS");
            TypeVar* var = rhsType->as<TypeVar>();
            if (!var->isRigid()) {
                lhsType = lhs.asType()->substitute(m_vm, m_substitutions);
                bind(var, lhsType);
            }
            return;
        }
    }

    // TODO: Is it safe to move this up? Find out and either move it or document why can't it be moved.
    if (lhs.isType())
        lhsType = lhs.asType()->substitute(m_vm, m_substitutions);

    // isType(σ) // implicit, rhs is always a type
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

    // Apply T-Binding-L again, since we're now looking at lhs.asType() rather than lhs.type(),
    // but don't wrap lhs in an AbstractValue, since it was already a type
    if (lhsType->is<TypeBinding>()) {
        TypeBinding* binding = lhsType->as<TypeBinding>();
        unifies(bytecodeOffset, binding->type(), rhs);
        return;
    }

    // ---------------------------------------- T-Top
    // σ <: ⊤
    if (rhsType->is<TypeTop>())
        return;

    // ---------------------------------------- T-Bottom
    // ⊥ <: σ
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
            unifies(bytecodeOffset, AbstractValue { collapsed }, rhs);
            return;

            // TODO: ∀σ, T | T <: σ => T <: σ
        }

        // S <: σ
        // T <: σ
        // ---------------------------------------- T-Union-L
        // ∀σ, S | T <: σ
        unifies(bytecodeOffset, AbstractValue { unionType->lhs().asType() }, rhs);
        unifies(bytecodeOffset, AbstractValue { unionType->rhs().asType() }, rhs);
        return;
    }

    if (rhsType->is<TypeUnion>()) {
        // σ <: S ∨ σ <: T
        // ---------------------------------------- T-Union-R
        // σ <: S | T
        auto* unionType = rhsType->as<TypeUnion>();
        unificationOr([&] {
            unifies(bytecodeOffset, lhs, unionType->lhs());
        }, [&] {
            unifies(bytecodeOffset, lhs, unionType->rhs());
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
                unifies(bytecodeOffset, AbstractValue { lhsItemsTypes->getIndex(i).asType() }, rhsItemsTypes->getIndex(i));
            return;
        }
        case Type::Class::Record: {
            TypeRecord* lhs = lhsType->as<TypeRecord>();
            TypeRecord* rhs = rhsType->as<TypeRecord>();
            if (lhs->size() < rhs->size())
                break;
            bool failed = false;
            for (const auto& pair : *rhs) {
                auto lhsField = lhs->field(pair.first);
                // TODO: better error message
                if (!lhsField) {
                    failed = true;
                    break;
                }
                unifies(bytecodeOffset, AbstractValue { lhsField }, pair.second);
            }
            if (!failed)
                return;
            break;
        }
        case Type::Class::Array: {
            TypeArray* lhs = lhsType->as<TypeArray>();
            TypeArray* rhs = rhsType->as<TypeArray>();
            unifies(bytecodeOffset, AbstractValue { lhs->itemType().asType()}, rhs->itemType());
            return;
        }
        case Type::Class::Function: {
            TypeFunction* lhs = lhsType->as<TypeFunction>();
            TypeFunction* rhs = rhsType->as<TypeFunction>();
            if (lhs->params()->size() != rhs->params()->size())
                break;
            for (uint32_t i = 0; i < lhs->params()->size(); i++)
                unifies(bytecodeOffset, rhs->param(i), lhs->param(i));
            unifies(bytecodeOffset, AbstractValue { lhs->returnType().asType() }, rhs->returnType());
            return;
        }
        case Type::Class::Hole: {
            Value lhsValue = partiallyEvaluate(Value { lhsType }, m_vm, m_environment);
            Value rhsValue = partiallyEvaluate(Value { rhsType }, m_vm, m_environment);
            if (lhsValue == rhsValue)
                return;
            goto unificationError;
        };
        default:
            break;
        }
    }

    // eval(<hole>) => T
    // S <: T
    // ---------------------------------------- T-Hole-R
    // Γ ⊢ S <: <hole>
    if (rhsType->is<Hole>()) {
        Type* type = eval(rhsType->as<Hole>());
        unifies(bytecodeOffset, lhs, type);
        return;
    }

    // eval(<hole>) => S
    // S <: T
    // ---------------------------------------- T-Hole-L
    // Γ ⊢ <hole> <: T
    if (lhsType->is<Hole>()) {
        Type* type = eval(lhsType->as<Hole>());
        unifies(bytecodeOffset, type, rhs);
        return;
    }

    if (rhsType->is<TypeVar>() && !rhsType->as<TypeVar>()->isRigid())
        rhsType = m_vm.typeType;

unificationError:
    std::stringstream msg;
    msg << "Unification failure: expected `" << *rhsType << "` but found `" << *lhsType << "`";
    m_unificationFailed = true;
    if (m_shouldThrowTypeError) {
        LOG(ConstraintSolving, msg.str() << " @ " << m_vm.currentBlock->locationInfo(bytecodeOffset));
        m_vm.typeError(bytecodeOffset, msg.str());
    }
}

void UnificationScope::matches(InstructionStream::Offset bytecodeOffset, Value scrutinee, Value pattern)
{
    LOG(ConstraintSolving, "Solving constraint: " << scrutinee << " `matches` " << pattern << " @ " << m_vm.currentBlock->locationInfo(bytecodeOffset));

    Type* scrutineeType = scrutinee.type(m_vm)->substitute(m_vm, m_substitutions);

    // Γ ⊢ T `matches` P
    // ---------------------------------------- M-Binding
    // Γ ⊢ x : T `matches` P
    if (scrutineeType->is<TypeBinding>()) {
        matches(bytecodeOffset, AbstractValue { scrutineeType->as<TypeBinding>()->type() }, pattern);
        return;
    }

    // Γ ⊢ eval(<hole>) `matches` P
    // ---------------------------------------- M-Hole
    // Γ ⊢ <hole> `matches` P
    if (scrutineeType->is<Hole>()) {
        Type* scrutinee = eval(scrutineeType->as<Hole>());
        matches(bytecodeOffset, AbstractValue { scrutinee }, pattern);
        return;
    }

    if (scrutineeType->is<TypeUnion>()) {
        TypeUnion* scrutineeUnion = scrutineeType->as<TypeUnion>();
        unificationOr([&] {
            // Γ ⊢ S `matches` P
            // ---------------------------------------- M-Union-L
            // Γ ⊢ S | T `matches` P
            matches(bytecodeOffset, AbstractValue { scrutineeUnion->lhs().asType() }, pattern);
        }, [&] {
            // Γ ⊢ T `matches` P
            // ---------------------------------------- M-Union-R
            // Γ ⊢ S | T `matches` P
            matches(bytecodeOffset, AbstractValue { scrutineeUnion->rhs().asType() }, pattern);
        });
        return;
    }

    // Γ ⊢ S <: P
    // ---------------------------------------- M-Sub
    // Γ ⊢ S `matches` P
    unifies(bytecodeOffset, scrutinee, pattern);
}

Type* UnificationScope::eval(Value hole)
{
    //if (m_vm.hasTypeErrors())
        //return m_vm.topType;
    BytecodeGenerator generator(m_vm);
    auto bytecode = holeCodegen(hole, generator);
    Value result = Interpreter::run(m_vm, *bytecode, m_environment);
    ASSERT(result.isType(), "OOPS");
    return result.asType()->substitute(m_vm, m_substitutions);
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
