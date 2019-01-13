#include "Array.h"

void Array::visit(std::function<void(Value)> visitor) const
{
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
