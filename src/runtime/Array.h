#pragma once

#include "Cell.h"
#include "VM.h"

class Array : public Cell {
public:
    CELL(Array)

    void setIndex(uint32_t index, Value item)
    {
        ASSERT(index < m_items.size(), "Array index out of bounds: %u", index);
        m_items[index] = item;
    }

    Value getIndex(Value indexValue)
    {
        uint32_t index = static_cast<uint32_t>(indexValue.asNumber());
        ASSERT(index < m_items.size(), "Array index out of bounds: %u", index);
        return m_items[index];
    }

    size_t size() { return m_items.size(); }
    std::vector<Value>::iterator begin() { return m_items.begin(); }
    std::vector<Value>::iterator end() { return m_items.end(); }
    std::vector<Value>::const_iterator begin() const { return m_items.begin(); }
    std::vector<Value>::const_iterator end() const { return m_items.end(); }

    void visit(std::function<void(Value)>) const override;
    void dump(std::ostream& out) const override;

private:
    Array(uint32_t initialSize)
        : m_items(initialSize)
    {
    }

    template<typename T>
    Array(const std::vector<T>& vector)
        : m_items(vector.begin(), vector.end())
    {
    }

    Array(uint32_t itemCount, const Value* items)
        : m_items(items, items + itemCount)
    {
    }

    std::vector<Value> m_items;
};

extern Array* createArray(VM&, uint32_t);
