#include "TypeChecker.h"

#include "AST.h"
#include <sstream>

const Type* TypeChecker::check(const std::unique_ptr<Program>& program)
{
    const Type* result = &unitType();
    for (const auto& decl : program->declarations) {
        result = &decl->infer(*this);
    }
    if (m_errors.size())
        return nullptr;
    return result;
}

TypeChecker::TypeChecker()
{
    insert("Void", newNameType("Void"));
    insert("Bool", newNameType("Bool"));
    insert("Number", newNameType("Number"));
    insert("String", newNameType("String"));
    insert("print", newFunctionType({ numericType() }, unitType()));
}

const Type& TypeChecker::unitType()
{
    return *m_types[0];
}

const Type& TypeChecker::booleanType()
{
    return *m_types[1];
}

const Type& TypeChecker::numericType()
{
    return *m_types[2];
}

const Type& TypeChecker::stringType()
{
    return *m_types[3];
}

const TypeName& TypeChecker::newNameType(const std::string& name)
{
    TypeName* nameType = new TypeName(m_types.size(), name);
    m_types.emplace_back(nameType);
    return *nameType;
}

const TypeFunction& TypeChecker::newFunctionType(Types params, const Type& returnType)
{
    TypeFunction* fnType = new TypeFunction(m_types.size(), params, returnType);
    m_types.emplace_back(fnType);
    return *fnType;
}

const TypeArray& TypeChecker::newArrayType(const Type& itemType)
{
    TypeArray* arrayType = new TypeArray(m_types.size(), itemType);
    m_types.emplace_back(arrayType);
    return *arrayType;
}

const Type& TypeChecker::lookup(const SourceLocation& location, const std::string& name)
{
    for (uint32_t i = m_environment.size(); i--;) {
        const auto& pair = m_environment[i];
        if (pair.first == name)
            return *m_types[pair.second];
    }
    std::stringstream msg;
    msg << "Unknown type: `" << name << "`";
    typeError(location, msg.str());
    return unitType();
}

void TypeChecker::insert(const std::string& name, const Type& type)
{
    m_environment.emplace_back(name, type.m_offset);
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

void TypeChecker::checkEquals(const SourceLocation& location, const Type& lhs, const Type& rhs)
{
    if (lhs == rhs)
        return;

    std::stringstream message;
    message << "Type mismatch: expected `" << rhs << "` but found `" << lhs << "`";
    m_errors.emplace_back(Error {
        location,
        message.str(),
    });
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
    , m_typesSize(tc.m_types.size())
{
}

TypeChecker::Scope::~Scope()
{
    m_typeChecker.m_environment.resize(m_environmentSize);
    for (uint32_t i = m_typesSize; i < m_typeChecker.m_types.size(); i++)
        delete m_typeChecker.m_types[i];
    m_typeChecker.m_types.resize(m_typesSize);
}
