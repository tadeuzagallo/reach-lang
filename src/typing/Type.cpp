#include "Type.h"

#include "Binding.h"
#include "TypeChecker.h"

bool Type::operator!=(const Type& other) const
{
    return !(*this == other);
}

const Type& Type::instantiate(TypeChecker& tc) const
{
    return *this;
}

std::ostream& operator<<(std::ostream& out, const Type& type)
{
    type.dump(out);
    return out;
}

void TypeType::visit(std::function<void(Value)>) const { };

const Type& TypeType::substitute(TypeChecker&, Substitutions&) const
{
    return *this;
}

bool TypeType::operator==(const Type& other) const
{
    return other.is<TypeType>();
}

void TypeType::dump(std::ostream& out) const
{
    out << "Type";
}

void TypeBottom::visit(std::function<void(Value)>) const { };

const Type& TypeBottom::substitute(TypeChecker&, Substitutions&) const
{
    return *this;
}

bool TypeBottom::operator==(const Type& other) const
{
    return other.is<TypeBottom>();
}

void TypeBottom::dump(std::ostream& out) const
{
    out << "⊥";
}

TypeName::TypeName(const std::string& name)
    : m_name(name)
{
}

void TypeName::visit(std::function<void(Value)>) const { }

const Type& TypeName::substitute(TypeChecker&, Substitutions&) const
{
    return *this;
}

bool TypeName::operator==(const Type& other) const
{
    if (!other.is<TypeName>())
        return false;
    return m_name == other.as<TypeName>().m_name;
}

void TypeName::dump(std::ostream& out) const
{
    out << m_name;
}

TypeFunction::TypeFunction(const Types& params, const Type& returnType)
    : m_params(params)
    , m_returnType(returnType)
{
}

size_t TypeFunction::paramCount() const
{
    return m_params.size();
}

const Binding& TypeFunction::param(uint32_t index) const
{
    ASSERT(index < paramCount(), "Out of bounds access to TypeFunction::param");
    return m_params[index];
}

const Type& TypeFunction::returnType() const
{
    return m_returnType;
}

const Type& TypeFunction::instantiate(TypeChecker& tc) const
{
    Substitutions subst;
    for (const Binding& param : m_params) {
        if (param.type().is<TypeType>()) {
            param.valueAsType().as<TypeVar>().fresh(tc, subst);
        }
    }
    return substitute(tc, subst);
}

void TypeFunction::visit(std::function<void(Value)> visitor) const
{
    for (const Binding& param : m_params)
        param.visit(visitor);
    visitor(Value { &m_returnType });
}

const Type& TypeFunction::substitute(TypeChecker& tc, Substitutions& subst) const
{
    Types params;
    for (const Binding& param : m_params)
        params.emplace_back(param.substitute(tc, subst));
    const Type& returnType = m_returnType.substitute(tc, subst);
    return *TypeFunction::create(tc.vm(), params, returnType);
}

bool TypeFunction::operator==(const Type& otherT) const
{
    if (!otherT.is<TypeFunction>())
        return false;
    const TypeFunction& other = otherT.as<TypeFunction>();
    if (m_params.size() != other.m_params.size())
        return false;
    for (unsigned i = 0; i < m_params.size(); i++)
        if (m_params[i] != other.m_params[i])
            return false;
    if (m_returnType != other.m_returnType)
        return false;
    return true;
}

void TypeFunction::dump(std::ostream& out) const
{
    out << "(";
    bool isFirst = true;
    for (const Binding& param : m_params) {
        if (!isFirst)
            out << ", ";
        out << param.type();
        isFirst = false;
    }
    out << ") -> " << m_returnType;
}

TypeArray::TypeArray(const Type& itemType)
    : m_itemType(itemType)
{
}

const Type& TypeArray::itemType() const
{
    return m_itemType;
}

void TypeArray::visit(std::function<void(Value)> visitor) const
{
    visitor(Value { &m_itemType });
}

const Type& TypeArray::substitute(TypeChecker& tc, Substitutions& subst) const
{
    const Type& itemType = m_itemType.substitute(tc, subst);
    return *TypeArray::create(tc.vm(), itemType);
}

bool TypeArray::operator==(const Type& other) const
{
    if (!other.is<TypeArray>())
        return false;
    return m_itemType == other.as<TypeArray>().m_itemType;
}

void TypeArray::dump(std::ostream& out) const
{
    out << m_itemType << "[]";
}

TypeRecord::TypeRecord(const Fields& fields)
    : m_fields(fields)
{
}

std::optional<std::reference_wrapper<const Binding>> TypeRecord::field(const std::string& name) const
{
    const auto it = m_fields.find(name);
    if (it == m_fields.end())
        return std::nullopt;
    return { it->second };
}

void TypeRecord::visit(std::function<void(Value)> visitor) const
{
    for (const auto& field : m_fields)
        field.second.visit(visitor);
}

const Type& TypeRecord::substitute(TypeChecker& tc, Substitutions& subst) const
{
    Fields fields;
    for (const auto& field : m_fields)
        fields.emplace(field.first, field.second.substitute(tc, subst));
    return *TypeRecord::create(tc.vm(), fields);
}

bool TypeRecord::operator==(const Type& otherT) const
{
    if (!otherT.is<TypeRecord>())
        return false;
    const TypeRecord& other = otherT.as<TypeRecord>();
    if (m_fields.size() != other.m_fields.size())
        return false;
    for (const auto& pair : m_fields) {
        auto otherField = other.field(pair.first);
        if (!otherField || pair.second != otherField)
            return false;
    }
    return true;
}

void TypeRecord::dump(std::ostream& out) const
{
    out << "{";
    bool isFirst = true;
    for (const auto& pair : m_fields) {
        if (!isFirst)
            out << ", ";
        out << pair.first << ": " << pair.second.type();
        isFirst = false;
    }
    out << "}";
}

uint32_t TypeVar::s_uid = 0;

TypeVar::TypeVar(const std::string& name)
    : m_uid(++s_uid)
    , m_name(name)
{
}

uint32_t TypeVar::uid() const
{
    return m_uid;
}

void TypeVar::fresh(TypeChecker& tc, Substitutions& subst) const
{
    const TypeVar& newVar = *TypeVar::create(tc.vm(), m_name);
    subst.emplace(m_uid, newVar);
}

void TypeVar::visit(std::function<void(Value)>) const { }

const Type& TypeVar::substitute(TypeChecker& tc, Substitutions& subst) const
{
    const auto it = subst.find(m_uid);
    if (it != subst.end())
        return it->second;
    return *this;
}

bool TypeVar::operator==(const Type& other) const
{
    if (!other.is<TypeVar>())
        return false;
    return m_uid == other.as<TypeVar>().m_uid;
}

void TypeVar::dump(std::ostream& out) const
{
    out << m_name << m_uid;
}
