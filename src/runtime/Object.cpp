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
        out << field.first << " = " << field.second;
        first = false;
    }
    out << "}";
}

Object* createObject(VM& vm, uint32_t inlineSize)
{
    return Object::create(vm, inlineSize);
}
