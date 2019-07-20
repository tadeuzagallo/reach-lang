#include "Array.h"

void Array::visit(std::function<void(Value)> visitor) const
{
    Typed::visit(visitor);
    for (auto item : m_items)
        visitor(item);
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
