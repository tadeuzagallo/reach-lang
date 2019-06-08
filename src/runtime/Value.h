#pragma once

#include "AbstractValue.h"
#include "Cell.h"
#include <iostream>

class Value {
    friend class Heap;
    friend class JIT;

public:
    static Value crash();
    static Value unit();

    Value(); // defaults to Value(Crash)
    Value(bool);
    Value(double);
    Value(uint32_t);
    Value(int32_t);
    Value(Cell*);
    Value(AbstractValue);

    bool isUnit() const;
    bool isNumber() const;
    bool isBool() const;
    bool isCell() const;
    bool isAbstractValue() const;
    bool isType() const;

    double asNumber() const;
    bool asBool() const;
    Cell* asCell() const;
    AbstractValue asAbstractValue() const;
    Type* asType() const;
    
    Type* type(VM&) const;

    template<typename T>
    T* asCell() const { return asCell()->cast<T>(); }

    bool operator!=(const Value&) const;
    bool operator==(const Value&) const;
    void dump(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& out, const Value& value)
    {
        value.dump(out);
        return out;
    }

private:
    static constexpr int64_t TagTypeAbstractValue = 0b1;
    static constexpr int64_t TagTypeBool = 0b10;
    static constexpr int64_t TagTypeUnit = 0b100;
    static constexpr int64_t TagTypeNumber = 0xffff000000000000;
    static constexpr int64_t TagMask = TagTypeNumber | TagTypeUnit | TagTypeBool | TagTypeAbstractValue;
    static constexpr int64_t DoubleEncodeOffset = 0x1000000000000;

    enum CrashTag { Crash };
    enum UnitTag { Unit };

    explicit Value(CrashTag);
    explicit Value(UnitTag);

    bool isCrash() const;
    Cell* getCell() const;

    int64_t m_bits;
};
