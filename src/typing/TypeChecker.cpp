#include "TypeChecker.h"

#include "AST.h"
#include "Type.h"
#include <sstream>

const Type* TypeChecker::check(const std::unique_ptr<Program>& program)
{
    const Type* result = &unitType();
    for (const auto& decl : program->declarations) {
        if (decl == program->declarations.back())
            result = &decl->infer(*this);
        else
            decl->check(*this, unitType());
    }
    if (m_errors.size())
        return nullptr;
    return result;
}

TypeChecker::TypeChecker()
{
    m_environment["Void"] = Type::named("Void");
    m_environment["Bool"] = Type::named("Bool");
    m_environment["Number"] = Type::named("Number");
    m_environment["String"] = Type::named("String");
}

const Type& TypeChecker::unitType()
{
    return lookup("Void");
}

const Type& TypeChecker::booleanType()
{
    return lookup("Bool");
}

const Type& TypeChecker::numericType()
{
    return lookup("Number");
}

const Type& TypeChecker::stringType()
{
    return lookup("String");
}

const Type& TypeChecker::lookup(const std::string& name)
{
    auto it = m_environment.find(name);
    ASSERT(it != m_environment.end(), "Unknown type: %s", name.c_str());
    return *it->second;
}

void TypeChecker::insert(const std::string& name, const Type* type)
{
    m_environment[name] = type;
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

