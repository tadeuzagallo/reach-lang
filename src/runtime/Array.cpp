#include "Array.h"

void Array::visit(const Visitor& visitor) const
{
    Typed::visit(visitor);
    for (auto item : m_items)
        visitor.visit(item);
}

void Array::dump(std::ostream& out) const
{
    out << "[";
    bool first = true;
    for (auto item : m_items) {
        if (!first)
            out << ", ";
        item.dump(out);
        first = false;
    }
    out << "]";
}

Array* createArray(VM& vm, Type* type, uint32_t inlineSize)
{
    return Array::create(vm, type, inlineSize);
}
