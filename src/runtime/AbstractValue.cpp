#include "AbstractValue.h"

#include "Type.h"

AbstractValue::AbstractValue(Type* type)
    : m_type(type)
{
}

Type* AbstractValue::type() const
{
    return m_type;
}

void AbstractValue::dump(std::ostream& out) const
{
    out << "AbstractValue { " << *m_type <<  " }";
}

uintptr_t AbstractValue::bits() const
{
    return reinterpret_cast<uintptr_t>(m_type);
}
