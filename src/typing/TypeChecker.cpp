#include "TypeChecker.h"

#include "AST.h"
#include "BytecodeGenerator.h"
#include "Log.h"
#include <sstream>
#include <limits>

TypeChecker::TypeChecker(BytecodeGenerator& generator)
    : m_generator(generator)
    //, m_topScope(*this)
    , m_topUnificationScope(*this)

    , m_typeType(generator.newLocal())
    , m_bottomType(generator.newLocal())
    , m_unitType(generator.newLocal())
    , m_boolType(generator.newLocal())
    , m_numberType(generator.newLocal())
    , m_stringType(generator.newLocal())
{
    ASSERT(!vm().typeChecker, "Already type checking");
    vm().typeChecker = this;

    m_generator.loadConstant(m_typeType, vm().typeType);
    m_generator.loadConstant(m_bottomType, vm().bottomType);
    m_generator.loadConstant(m_unitType, vm().unitType);
    m_generator.loadConstant(m_boolType, vm().boolType);
    m_generator.loadConstant(m_numberType, vm().numberType);
    m_generator.loadConstant(m_stringType, vm().stringType);

    // No need to insert Type since it's a reserved keyword
    insert("Void", m_unitType);
    insert("Bool", m_boolType);
    insert("Number", m_numberType);
    insert("String", m_stringType);
}

Register TypeChecker::check(const std::unique_ptr<Program>& program)
{
    Register result = m_generator.newLocal();
    //unitValue(result);
    for (const auto& decl : program->declarations) {
        decl->infer(*this, result);
    }
    //m_topUnificationScope.resolve(result);
    // TODO: error checkpoint
    return result;
}

void TypeChecker::visit(const std::function<void(Value)>& visitor) const
{
    // TODO
}

VM& TypeChecker::vm() const
{
    return m_generator.vm();
}

BytecodeGenerator& TypeChecker::generator()
{
    return m_generator;
}

Register TypeChecker::typeType()
{
    return m_typeType;
}

Register TypeChecker::unitType()
{
    return m_unitType;
}

Register TypeChecker::numberType()
{
    return m_numberType;
}

void TypeChecker::typeType(Register result)
{
    m_generator.move(result, m_typeType);
}

void TypeChecker::unitType(Register result)
{
    m_generator.move(result, m_unitType);
}

void TypeChecker::bottomValue(Register result)
{
    m_generator.newValue(result, m_bottomType);
}

void TypeChecker::unitValue(Register result)
{
    m_generator.newValue(result, m_unitType);
}

void TypeChecker::booleanValue(Register result)
{
    m_generator.newValue(result, m_boolType);
}

void TypeChecker::numberValue(Register result)
{
    m_generator.newValue(result, m_numberType);
}

void TypeChecker::stringValue(Register result)
{
    m_generator.newValue(result, m_stringType);
}

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
    // TODO: location
    m_generator.getType(result, name);
}

void TypeChecker::insert(const std::string& name, Register type)
{
    m_generator.setType(name, type);
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
    : m_typeChecker(typeChecker)
{
    m_typeChecker.m_generator.pushUnificationScope();
}

TypeChecker::UnificationScope::~UnificationScope()
{
    m_typeChecker.m_generator.popUnificationScope();
}

void TypeChecker::UnificationScope::resolve(Register dst, Register type)
{
    m_typeChecker.m_generator.resolveType(dst, type);
}
