#include "Type.h"

Type::Type(Tag tag)
    : m_tag(tag)
{
}

TypeName::TypeName(const std::string& name)
    : Type(Tag::Name)
    , m_name(name)
{
}

bool TypeName::operator==(const TypeName& other) const
{
    return m_name == other.m_name;
}

void TypeName::dump(std::ostream& out) const
{
    out << m_name;
}

TypeFunction::TypeFunction(Types params, const Type& returnType)
    : Type(Tag::Function)
    , m_params(params)
    , m_returnType(returnType)
{
}

size_t TypeFunction::paramCount() const
{
    return m_params.size();
}

const Type& TypeFunction::param(uint32_t index) const
{
    ASSERT(index < paramCount(), "Out of bounds access to TypeFunction::param");
    return m_params[index];
}

const Type& TypeFunction::returnType() const
{
    return m_returnType;
}

bool TypeFunction::operator==(const TypeFunction& other) const
{
    if (m_params.size() != other.m_params.size())
        return false;
    for (unsigned i = 0; i < m_params.size(); i++)
        if (m_params[i].get() != other.m_params[i].get())
            return false;
    if (m_returnType != other.m_returnType)
        return false;
    return true;
}

void TypeFunction::dump(std::ostream& out) const
{
    out << "(";
    bool isFirst = true;
    for (const Type& param : m_params) {
        if (!isFirst)
            out << ", ";
        out << param;
        isFirst = false;
    }
    out << ") -> " << m_returnType;
}

Type* Type::named(const std::string& name)
{
    return new TypeName(name);
}

Type* Type::function(Types params, const Type& returnType)
{
    return new TypeFunction(params, returnType);
}

bool Type::operator==(const Type& other) const
{
    if (m_tag != other.m_tag)
        return false;
    switch (m_tag) {
    case Tag::Name:
        return asName() == other.asName();
    case Tag::Function:
        return asFunction() == other.asFunction();
    }
}

bool Type::operator!=(const Type& other) const
{
    return !(*this == other);
}

bool Type::isName() const
{
    return m_tag == Tag::Name;
}

bool Type::isFunction() const
{
    return m_tag == Tag::Function;
}

const TypeName& Type::asName() const
{
    ASSERT(m_tag == Tag::Name, "Invalid conversion: type is not a name");
    return static_cast<const TypeName&>(*this);

}

const TypeFunction& Type::asFunction() const
{
    ASSERT(m_tag == Tag::Function, "Invalid conversion: type is not a function");
    return static_cast<const TypeFunction&>(*this);

}

void Type::dump(std::ostream& out) const
{
    switch (m_tag) {
    case Tag::Name:
        asName().dump(out);
        break;
    case Tag::Function:
        asFunction().dump(out);
        break;
    }
}

std::ostream& operator<<(std::ostream& out, const Type& type)
{
    type.dump(out);
    return out;
}
