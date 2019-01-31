#include "Binding.h"

#include "Assert.h"
#include "Cell.h"

bool Binding::valueIsType() const
{
    if (!m_value.isCell())
        return false;
    return m_value.asCell()->is<Type>();
}

const Type& Binding::valueAsType() const
{
    return *m_value.asCell()->cast<Type>();
}

Binding Binding::substitute(TypeChecker& tc, Substitutions& subst) const
{
    Value value = m_value;
    if (value.isCell()) {
        Cell* cell = value.asCell();
        if (cell->is<Type>()) {
            const Type& ty = cell->cast<Type>()->substitute(tc, subst);
            value = Value { &ty };
        }
    }
    return Binding { value, m_type.substitute(tc, subst) };
}

bool Binding::inferred() const
{
    if (!m_value.isCell())
        return false;
    Cell* cell = m_value.asCell();
    if (!cell->is<Type>())
        return false;
    const Type& type = *cell->cast<Type>();
    if (!type.is<TypeVar>())
        return false;
    return type.as<TypeVar>().inferred();
}

bool Binding::operator!=(const Binding& other) const
{
    return !(*this == other);
}

bool Binding::operator==(const Binding& other) const
{
    return m_value == other.m_value && m_type == other.m_type;
}

void Binding::visit(std::function<void(Value)> visitor) const
{
    visitor(m_value);
    visitor(Value { &m_type });
}
