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
    , m_topScope(*this, false)
    , m_topUnificationScope(*this)
{
    m_currentScope = &m_topScope;
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
    if (!program.declarations.size())
        unitValue(result);
    endTypeChecking(Mode::Program, result);
}

VM& TypeChecker::vm() const
{
    return m_generator.vm();
}

TypeChecker* TypeChecker::previousTypeChecker() const
{
    return m_previousTypeChecker;
}

auto TypeChecker::currentScope() const -> Scope&
{
    return *m_currentScope;
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
        break;
    }
    m_topUnificationScope.finalize();
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

void TypeChecker::newArrayType(Register result, Register itemType)
{
    m_generator.newArrayType(result, itemType);
}

void TypeChecker::newRecordType(Register result, const std::vector<std::pair<std::string, Register>>& fields)
{
    m_generator.newRecordType(result, fields);
}

void TypeChecker::lookup(Register result, const SourceLocation& location, const std::string& name)
{
    m_generator.emitLocation(location);
    m_generator.getLocal(result, name);
}

void TypeChecker::insert(const std::string& name, Register type)
{
    m_generator.setLocal(name, type);
}

void TypeChecker::unify(const SourceLocation& location, Register lhs, Register rhs)
{
    LOG(ConstraintSolving, "unify: " << location << ": " << lhs << " U " << rhs);
    m_generator.emitLocation(location);
    m_generator.unify(lhs, rhs);
}

TypeChecker::Scope::Scope(TypeChecker& typeChecker, bool shouldGenerateBytecode)
    : m_shouldGenerateBytecode(shouldGenerateBytecode)
    , m_typeChecker(typeChecker)
{
    m_previousScope = m_typeChecker.m_currentScope;
    m_typeChecker.m_currentScope = this;
    if (m_shouldGenerateBytecode)
        m_typeChecker.m_generator.pushScope();
}

TypeChecker::Scope::~Scope()
{
    m_typeChecker.m_currentScope = m_previousScope;
    if (m_shouldGenerateBytecode)
        m_typeChecker.m_generator.popScope();
}

void TypeChecker::Scope::addFunction(const FunctionDeclaration& decl)
{
    std::vector<bool> parameters(decl.parameters.size(), false);
    bool hasInferredArguments = false;
    for (uint32_t i = 0; i < decl.parameters.size(); i++) {
        if (decl.parameters[i]->inferred) {
            parameters[i] = true;
            hasInferredArguments = true;
        }
    }
    if (hasInferredArguments)
        m_functions.emplace(decl.name->name, std::move(parameters));
}

std::optional<std::vector<bool>> TypeChecker::Scope::getFunction(const std::string& name)
{
    const auto& it = m_functions.find(name);
    if (it != m_functions.end())
        return it->second;
    if (m_previousScope)
        return m_previousScope->getFunction(name);
    return std::nullopt;
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
