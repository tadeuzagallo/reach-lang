#pragma once

#include "Cell.h"
#include "VM.h"

class Array : public Cell {
public:
    CELL(Array)

    void setIndex(uint32_t index, Value item)
    {
        ASSERT(index < m_items.size(), "Array ndex out of bounds: %u", index);
        m_items[index] = item;
    }

    Value getIndex(uint32_t index)
    {
        ASSERT(index < m_items.size(), "Array ndex out of bounds: %u", index);
        return m_items[index];
    }

    void visit(std::function<void(Value)>) const override;
    void dump(std::ostream& out) const override;

private:
    Array(uint32_t initialSize)
        : m_items(initialSize)
    {
    }

    std::vector<Value> m_items;
};
