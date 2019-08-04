#pragma once

#include "Typed.h"
#include "VM.h"

class Array : public Typed {
public:
    CELL(Array)

    void setIndex(uint32_t index, Value item)
    {
        ASSERT(index < m_items.size(), "Array index out of bounds: %u", index);
        m_items[index] = item;
    }

    Value getIndex(Value indexValue) const
    {
        uint32_t index = static_cast<uint32_t>(indexValue.asNumber());
        return getIndex(index);
    }

    Value getIndex(uint32_t index) const
    {
        ASSERT(index < m_items.size(), "Array index out of bounds: %u", index);
        return m_items[index];
    }

    size_t size() const { return m_items.size(); }
    std::vector<Value>::iterator begin() { return m_items.begin(); }
    std::vector<Value>::iterator end() { return m_items.end(); }
    std::vector<Value>::const_iterator begin() const { return m_items.begin(); }
    std::vector<Value>::const_iterator end() const { return m_items.end(); }

    bool operator==(const Array&) const;
    Array* substitute(VM&, const Substitutions&) const;

    void dump(std::ostream& out) const override;

protected:
    void visit(const Visitor&) const override;

    Array(Type* type, uint32_t initialSize)
        : Typed(type)
        , m_items(initialSize)
    {
    }

    template<typename T>
    Array(Type* type, const std::vector<T>& vector)
        : Typed(type)
        , m_items(vector.begin(), vector.end())
    {
    }

private:
    Array(Type* type, uint32_t itemCount, const Value* items)
        : Typed(type)
        , m_items(items, items + itemCount)
    {
    }

    std::vector<Value> m_items;
};

extern Array* createArray(VM&, Type*, uint32_t);
