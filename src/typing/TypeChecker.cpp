#include "TypeChecker.h"

#include "AST.h"
#include "Type.h"
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
    m_environment["Void"] = Type::named("Void");
    m_environment["Bool"] = Type::named("Bool");
    m_environment["Number"] = Type::named("Number");
    m_environment["String"] = Type::named("String");

    m_environment["print"] = Type::function({ numericType() }, unitType());
}

const Type& TypeChecker::unitType()
{
    return *m_environment["Void"];
}

const Type& TypeChecker::booleanType()
{
    return *m_environment["Bool"];
}

const Type& TypeChecker::numericType()
{
    return *m_environment["Number"];
}

const Type& TypeChecker::stringType()
{
    return *m_environment["String"];
}

const Type& TypeChecker::lookup(const SourceLocation& location, const std::string& name)
{
    auto it = m_environment.find(name);
    if (it == m_environment.end()) {
        std::stringstream msg;
        msg << "Unknown type: `" << name << "`";
        typeError(location, msg.str());
        return unitType();
    }
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

