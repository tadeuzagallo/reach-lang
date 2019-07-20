#include "Typed.h"

#include "Type.h"


Typed::Typed(Type* type)
 : m_type(type)
{
}

Type* Typed::type() const
{
    return m_type;
}

void Typed::visit(std::function<void(Value)> visitor) const
{
    visitor(m_type);
}
