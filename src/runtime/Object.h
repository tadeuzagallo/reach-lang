#pragma once

#include "Cell.h"
#include "VM.h"
#include <optional>
#include <unordered_map>

class Object : public Cell {
public:
    CELL(Object)

    void set(const std::string& field, Value value)
    {
        m_fields.emplace(field, value);
    }

    Value get(const std::string& field) const
    {
        auto it = m_fields.find(field);
        ASSERT(it != m_fields.end(), "Unknown field: %s", field.c_str());
        return it->second;
    }

    std::optional<Value> tryGet(const std::string& field) const
    {
        auto it = m_fields.find(field);
        if (it == m_fields.end())
            return std::nullopt;
        return { it->second };
    }

    size_t size() const { return m_fields.size(); }
    std::unordered_map<std::string, Value>::iterator begin() { return m_fields.begin(); }
    std::unordered_map<std::string, Value>::iterator end() { return m_fields.end(); }

    void visit(std::function<void(Value)>) const override;
    void dump(std::ostream& out) const override;

protected:
    Object(uint32_t inlineSize)
    {
        (void)inlineSize; // TODO
    }

    template<typename T>
    Object(const std::unordered_map<std::string, T>& fields)
     : m_fields(fields.begin(), fields.end())
    {
    }

private:
    std::unordered_map<std::string, Value> m_fields;
};

extern Object* createObject(VM&, uint32_t);
