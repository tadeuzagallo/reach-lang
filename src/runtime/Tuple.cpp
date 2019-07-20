#include "Tuple.h"

void Tuple::visit(std::function<void(Value)> visitor) const
{
    Typed::visit(visitor);
    for (auto item : m_items)
        visitor(item);
}

void Tuple::dump(std::ostream& out) const
{
    out << "(";
    bool first = true;
    for (auto item : m_items) {
        if (!first)
            out << ", ";
        item.dump(out);
        first = false;
    }
    out << ")";
}

Tuple* createTuple(VM& vm, Type* type, uint32_t inlineSize)
{
    return Tuple::create(vm, type, inlineSize);
}
