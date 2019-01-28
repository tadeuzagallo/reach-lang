#pragma once

#include "Type.h"
#include "Value.h"

// Only Type bindings have a value during type checking
// Value bindings have only type, value will return null;

class Binding {
    friend class TypeChecker;

public:
    Value value() const { return m_value; }
    const Type& type() const { return m_type; }

    const Type& valueAsType() const;
    Binding substitute(TypeChecker&, Substitutions&) const;
    bool inferred() const;

    bool operator!=(const Binding&) const;
    bool operator==(const Binding&) const;
    void visit(std::function<void(Value)>) const;

private:
    Binding(const Value& value, const Type& type)
        : m_value(value)
        , m_type(type)
    {
    }

    Binding(const Type& type)
        : m_value(Value::unit())
        , m_type(type)
    {
    }

    Value m_value;
    const Type& m_type;
};
