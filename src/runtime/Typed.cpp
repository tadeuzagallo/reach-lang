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

void Typed::visit(const Visitor& visitor) const
{
    visitor.visit(m_type);
}
