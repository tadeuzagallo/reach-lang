#include "TypeChecker.h"

#include "AST.h"
#include "Binding.h"
#include "Log.h"
#include <sstream>
#include <limits>

TypeChecker::TypeChecker(VM& vm)
    : m_vm(vm)
    , m_topScope(*this)
    , m_topUnificationScope(*this)
    , m_unificationScope(&m_topUnificationScope)
{
    ASSERT(!m_vm.typeChecker, "Already type checking");
    m_vm.typeChecker = this;
    Type* typeType = TypeType::create(vm);
    Type* typeBottom = TypeBottom::create(vm);
    m_bindings.emplace_back(new Binding { Value { typeType }, *typeType });
    m_bindings.emplace_back(new Binding { Value { typeBottom }, *typeType });
    insert("Void", newNameType("Void"));
    insert("Bool", newNameType("Bool"));
    insert("Number", newNameType("Number"));
    insert("String", newNameType("String"));

    insert("inspect", bottomValue());
    insert("print", bottomValue());
}

const Type* TypeChecker::check(const std::unique_ptr<Program>& program)
{
    const Binding* binding = &unitValue();
    for (const auto& decl : program->declarations) {
        binding = &decl->infer(*this);
        decl->binding = &newType(*binding);
    }
    const Type& result = m_topUnificationScope.resolve(binding->type());
    if (m_errors.size())
        return nullptr;
    return &result;
}

void TypeChecker::visit(const std::function<void(Value)>& visitor) const
{
    for (Binding* binding : m_bindings)
        binding->visit(visitor);
}

VM& TypeChecker::vm() const
{
    return m_vm;
}

const Binding& TypeChecker::typeType() { return *m_bindings[0]; }
const Binding& TypeChecker::unitType() { return *m_bindings[2]; }

const Binding& TypeChecker::bottomValue() { return newValue(*m_bindings[1]); }
const Binding& TypeChecker::unitValue() { return newValue(*m_bindings[2]); }
const Binding& TypeChecker::booleanValue() { return newValue(*m_bindings[3]); }
const Binding& TypeChecker::numericValue() { return newValue(*m_bindings[4]); }
const Binding& TypeChecker::stringValue() { return newValue(*m_bindings[5]); }

// New type bindings - for constructs that introduces new types, e.g.
// T : Type
const Binding& TypeChecker::newType(const Type& type)
{
    // introduce a binding for a new value of the given type
    m_bindings.emplace_back(new Binding { Value { &type }, typeType().type() });
    return *m_bindings.back();
}

const Binding& TypeChecker::newType(const Binding& binding)
{
    return newType(binding.type());
}

// New value bindings - for constructs that introduces new values, e.g.
// T : Type -> x : T
const Binding& TypeChecker::newValue(const Type& type)
{
    // introduce a binding for a new value of the given type
    m_bindings.emplace_back(new Binding { type });
    return *m_bindings.back();
}

// introduce a binding for a new value of the given type
const Binding& TypeChecker::newValue(const Binding& binding)
{
    // typeType is a special case, since it's recursive
    if (&binding == &typeType())
        return typeType();
    return newValue(binding.valueAsType());
}

const Binding& TypeChecker::newFunctionValue(const Types& params, const Type& returnType)
{
    TypeFunction* fnType = TypeFunction::create(m_vm, params, returnType);
    m_bindings.emplace_back(new Binding { *fnType });
    return *m_bindings.back();
}

const Binding& TypeChecker::newArrayValue(const Type& itemType)
{
    TypeArray* arrayType = TypeArray::create(m_vm, itemType);
    m_bindings.emplace_back(new Binding { *arrayType });
    return *m_bindings.back();
}

const Binding& TypeChecker::newRecordValue(const Fields& fields)
{
    TypeRecord* recordType = TypeRecord::create(m_vm, fields);
    m_bindings.emplace_back(new Binding { *recordType });
    return *m_bindings.back();
}

// New type bindings - for constructs that introduces new types, e.g.
// T : Type

const Binding& TypeChecker::newNameType(const std::string& name)
{
    TypeName* nameType = TypeName::create(m_vm, name);
    m_bindings.emplace_back(new Binding { Value { nameType }, typeType().type() });
    m_vm.addType(name, *nameType);
    return *m_bindings.back();
}

const Binding& TypeChecker::newVarType(const std::string& name, bool inferred)
{
    TypeVar* varType = TypeVar::create(m_vm, name, inferred);
    m_bindings.emplace_back(new Binding { Value { varType }, typeType().type() });
    return *m_bindings.back();
}

const Binding& TypeChecker::lookup(const SourceLocation& location, const std::string& name, const Binding& defaultValue)
{
    for (uint32_t i = m_environment.size(); i--;) {
        const auto& pair = m_environment[i];
        if (pair.first == name)
            return *pair.second;
    }

    std::stringstream msg;
    msg << "Unknown variable: `" << name << "`";
    typeError(location, msg.str());
    return defaultValue;
}

void TypeChecker::insert(const std::string& name, const Binding& binding)
{
    m_environment.emplace_back(name, &binding);
}

// Error handling
TypeChecker::Error::Error(const SourceLocation& location, const std::string& message)
    : m_location(location)
    , m_message(message)
{
}

const SourceLocation& TypeChecker::Error::location() const
{
    return m_location;
}

const std::string& TypeChecker::Error::message() const
{
    return m_message;
}

void TypeChecker::unify(const SourceLocation& location, const Binding& lhs, const Binding& rhs)
{
    LOG(ConstraintSolving, "unify: " << location << ": " << lhs.type() << " U " << rhs.type());
    m_unificationScope->unify(location, lhs, rhs);
}

void TypeChecker::typeError(const SourceLocation& location, const std::string& message)
{
    m_errors.emplace_back(Error {
        location,
        message,
    });
}

void TypeChecker::reportErrors(std::ostream& out) const
{
    for (const auto& error : m_errors) {
        out << error.location() << ": " << error.message() << std::endl;
    }
}


TypeChecker::Scope::Scope(TypeChecker& tc)
    : m_typeChecker(tc)
    , m_environmentSize(tc.m_environment.size())
    , m_bindingsSize(tc.m_bindings.size())
{
}

TypeChecker::Scope::~Scope()
{
    m_typeChecker.m_environment.resize(m_environmentSize);
    //for (uint32_t i = m_bindingsSize; i < m_typeChecker.m_bindings.size(); i++)
        //delete m_typeChecker.m_bindings[i];
    m_typeChecker.m_bindings.resize(m_bindingsSize);
}

TypeChecker::UnificationScope::UnificationScope(TypeChecker& tc)
    : m_typeChecker(tc)
    , m_parentScope(tc.m_unificationScope)
{
    LOG(UnificationScope, "===> UnificationScope <===");
    m_typeChecker.m_unificationScope = this;
}

TypeChecker::UnificationScope::~UnificationScope()
{
    finalize();
    m_typeChecker.m_unificationScope = m_parentScope;
    LOG(UnificationScope,  "===> ~UnificationScope <===");
}

void TypeChecker::UnificationScope::unify(const SourceLocation& location, const Binding& lhs, const Binding& rhs)
{
    ASSERT(!m_finalized, "UnificationScope already finalized");
    m_constraints.emplace_back(Constraint { location, lhs, rhs });
}

const Type& TypeChecker::UnificationScope::resolve(const Type& resultType)
{
    finalize();
    return resultType.substitute(m_typeChecker, m_substitutions);
}

void TypeChecker::UnificationScope::infer(const SourceLocation& location, const Binding& binding)
{
    ASSERT(binding.valueAsType().is<TypeVar>(), "");
    m_inferredBindings.emplace_back(InferredBinding { location, binding.valueAsType().as<TypeVar>() });
}

void TypeChecker::UnificationScope::finalize()
{
    if (m_finalized)
        return;
    m_finalized = true;
    solveConstraints();
    checkInferredVariables();
}

void TypeChecker::UnificationScope::solveConstraints()
{
    while (!m_constraints.empty()) {
        auto constraint = m_constraints.front();
        m_constraints.pop_front();


        const Type& lhs_ = constraint.lhs.type().substitute(m_typeChecker, m_substitutions);
        const Type& rhs_ = constraint.rhs.type().substitute(m_typeChecker, m_substitutions);

        LOG(ConstraintSolving, "Solving constraint: " << constraint.location << ": " << constraint.lhs.type() << " => " << lhs_ << " U " << constraint.rhs.type() << " => " << rhs_);
        unifies(constraint.location, lhs_, rhs_);
        if (lhs_.is<TypeType>() && rhs_.is<TypeType>()) {
            const Type& lhsValue = constraint.lhs.valueAsType();
            const Type& rhsValue = constraint.rhs.valueAsType();
            if (rhsValue.is<TypeVar>())
                bind(rhsValue.as<TypeVar>(), lhsValue);
        }
    }
}

void TypeChecker::UnificationScope::checkInferredVariables()
{
    for (const InferredBinding& ib : m_inferredBindings) {
        auto it = m_substitutions.find(ib.var.uid());
        if (it != m_substitutions.end())
            continue;
        std::stringstream message;
        message << "Unification failure: failed to infer type variable `" << ib.var << "`";
        m_typeChecker.typeError(ib.location, message.str());
    }
}

void TypeChecker::UnificationScope::unifies(const SourceLocation& location, const Type& lhs, const Type& rhs)
{
    if (lhs == rhs)
        return;
    if (rhs.is<TypeVar>()) {
        const TypeVar& var = rhs.as<TypeVar>();
        if (var.inferred()) {
            bind(var, lhs);
            return;
        }
    }

    std::stringstream msg;
    msg << "Unification failure: expected `" << rhs << "` but found `" << lhs << "`";
    m_typeChecker.typeError(location, msg.str());
}

void TypeChecker::UnificationScope::bind(const TypeVar& var, const Type& type)
{
    LOG(ConstraintSolving, "Binding " << var << " to " << type);
    m_substitutions.emplace(var.uid(), type);
}
