#include "Type.h"

Type::Type(Tag tag, uint32_t offset)
    : m_tag(tag)
    , m_offset(offset)
{
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
    case Tag::Array:
        return asArray() == other.asArray();
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

bool Type::isArray() const
{
    return m_tag == Tag::Array;
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

const TypeArray& Type::asArray() const
{
    ASSERT(m_tag == Tag::Array, "Invalid conversion: type is not a array");
    return static_cast<const TypeArray&>(*this);

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
    case Tag::Array:
        asArray().dump(out);
        break;
    }
}

std::ostream& operator<<(std::ostream& out, const Type& type)
{
    type.dump(out);
    return out;
}

TypeName::TypeName(uint32_t offset, const std::string& name)
    : Type(Tag::Name, offset)
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

TypeFunction::TypeFunction(uint32_t offset, Types params, const Type& returnType)
    : Type(Tag::Function, offset)
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

TypeArray::TypeArray(uint32_t offset, const Type& itemType)
    : Type(Tag::Array, offset)
    , m_itemType(itemType)
{
}

const Type& TypeArray::itemType() const
{
    return m_itemType;
}

bool TypeArray::operator==(const TypeArray& other) const
{
    return m_itemType == other.m_itemType;
}

void TypeArray::dump(std::ostream& out) const
{
    out << m_itemType << "[]";
}
