#pragma once

#include <iostream>

class Type;
class AbstractValue {
    friend class Value;

public:
    explicit AbstractValue(Type*);

    Type* type() const;
    void dump(std::ostream&) const;

private:
    uintptr_t bits() const;

    Type* m_type;
};
