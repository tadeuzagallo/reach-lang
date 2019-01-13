#pragma once

#include "Cell.h"
#include "VM.h"

class Object : public Cell {
public:
    CELL(Object)

    void set(const Identifier& field, Value value)
    {
        m_fields[field] = value;
    }

    Value get(const Identifier& field)
    {
        auto it = m_fields.find(field);
        ASSERT(it != m_fields.end(), "Unknown field: %s", field.name.c_str());
        return it->second;
    }

    void visit(std::function<void(Value)>) const override;
    void dump(std::ostream& out) const override;

private:
    Object(uint32_t inlineSize)
    {
        (void)inlineSize; // TODO
    }

    std::map<Identifier, Value> m_fields;
};
