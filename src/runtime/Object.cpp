#include "Object.h"

void Object::visit(std::function<void(Value)> visitor) const
{
    for (auto field : m_fields)
        visitor(field.second);
}

void Object::dump(std::ostream& out) const
{
    out << "{";
    bool first = true;
    for (const auto& field : m_fields) {
        if (!first)
            out << ", ";
        out << field.first.name << " = " << field.second;
        first = false;
    }
    out << "}";
}
