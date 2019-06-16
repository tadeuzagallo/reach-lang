#include "TypeChecker.h"

#include "AST.h"
#include "BytecodeGenerator.h"
#include "Log.h"
#include <sstream>
#include <limits>

void Program::typecheck(BytecodeGenerator& generator)
{
    TypeChecker checker(generator);
    checker.check(*this);
}

TypeChecker::TypeChecker(BytecodeGenerator& generator)
    : m_generator(generator)
    //, m_topScope(*this)
    , m_topUnificationScope(*this)
{
    m_previousTypeChecker = vm().typeChecker;
    vm().typeChecker = this;
}

TypeChecker::~TypeChecker()
{
    m_topUnificationScope.finalize();
    vm().typeChecker = m_previousTypeChecker;
}

void TypeChecker::check(Program& program)
{
    Register result = m_generator.newLocal();
    for (const auto& decl : program.declarations)
        decl->infer(*this, result);
    endTypeChecking(Mode::Program, result);
}

void TypeChecker::visit(const std::function<void(Value)>& visitor) const
{
    // TODO
}

VM& TypeChecker::vm() const
{
    return m_generator.vm();
}

TypeChecker* TypeChecker::previousTypeChecker() const
{
    return m_previousTypeChecker;
}

BytecodeGenerator& TypeChecker::generator() const
{
    return m_generator;
}

void TypeChecker::endTypeChecking(Mode mode, Register result)
{
    switch (mode) {
    case Mode::Program:
        m_generator.getTypeForValue(result, result);
        m_topUnificationScope.resolve(result, result);
        break;
    case Mode::Function:
        m_topUnificationScope.finalize();
        break;
    }
    m_generator.endTypeChecking(result);
}


#define IMPLEMENT_LAZY_TYPE_GETTER(type) \
    Register TypeChecker::type##Type() \
    { \
        if (!m_##type##Type) { \
            m_##type##Type = m_generator.newLocal(); \
            m_generator.emitPrologue([&] { \
                m_generator.loadConstant(m_##type##Type.value(), vm().type##Type); \
            }); \
        } \
        return m_##type##Type.value(); \
    } \

FOR_EACH_BASE_TYPE(IMPLEMENT_LAZY_TYPE_GETTER)
#undef IMPLEMENT_LAZY_TYPE_GETTER

#define IMPLEMENT_TYPE_VALUE_GETTER(type) \
    void TypeChecker::type##Value(Register result) \
    { \
        m_generator.newValue(result, type##Type()); \
    } \

FOR_EACH_BASE_TYPE(IMPLEMENT_TYPE_VALUE_GETTER)
#undef IMPLEMENT_TYPE_VALUE_GETTER

// New value bindings - for constructs that introduces new values, e.g.
// T : Type -> x : T
void TypeChecker::newValue(Register result, Register type)
{
    m_generator.newValue(result, type);
}

void TypeChecker::newFunctionValue(Register result, const std::vector<Register>& params, Register returnType)
{
    newFunctionType(result, params, returnType);
    m_generator.newValue(result, result);
}

void TypeChecker::newArrayValue(Register result, Register itemType)
{
    newArrayType(result, itemType);
    m_generator.newValue(result, result);
}

void TypeChecker::newRecordValue(Register result, const std::vector<std::pair<std::string, Register>>& fields)
{
    newRecordType(result, fields);
    m_generator.newValue(result, result);
}

// New type bindings - for constructs that introduces new types, e.g.
// T : Type

void TypeChecker::newNameType(Register result, const std::string& name)
{
    m_generator.newNameType(result, name);
}

void TypeChecker::newVarType(Register result, const std::string& name, bool inferred)
{
    m_generator.newVarType(result, name, inferred);
}

void TypeChecker::newArrayType(Register result, Register itemType)
{
    m_generator.newArrayType(result, itemType);
}

void TypeChecker::newRecordType(Register result, const std::vector<std::pair<std::string, Register>>& fields)
{
    m_generator.newRecordType(result, fields);
}

void TypeChecker::newFunctionType(Register result, const std::vector<Register>& params, Register returnType)
{
    m_generator.newFunctionType(result, params, returnType);
}

void TypeChecker::lookup(Register result, const SourceLocation& location, const std::string& name)
{
    m_generator.getLocal(result, name);
}

void TypeChecker::insert(const std::string& name, Register type)
{
    m_generator.setLocal(name, type);
}

void TypeChecker::unify(const SourceLocation& location, Register lhs, Register rhs)
{
    LOG(ConstraintSolving, "unify: " << location << ": " << lhs << " U " << rhs);
    // TODO: location
    m_generator.unify(lhs, rhs);
}

TypeChecker::Scope::Scope(TypeChecker& typeChecker)
    : m_typeChecker(typeChecker)
{
    m_typeChecker.m_generator.pushScope();
}

TypeChecker::Scope::~Scope()
{
    m_typeChecker.m_generator.popScope();
}

TypeChecker::UnificationScope::UnificationScope(TypeChecker& typeChecker)
    : m_typeChecker(&typeChecker)
{
    m_typeChecker->m_generator.pushUnificationScope();
}

TypeChecker::UnificationScope::~UnificationScope()
{
    finalize();
}

void TypeChecker::UnificationScope::resolve(Register dst, Register type)
{
    ASSERT(m_typeChecker, "OOPS");
    m_typeChecker->m_generator.resolveType(dst, type);
}

void TypeChecker::UnificationScope::finalize()
{
    if (!m_typeChecker)
        return;
    m_typeChecker->m_generator.popUnificationScope();
    m_typeChecker = nullptr;
}
