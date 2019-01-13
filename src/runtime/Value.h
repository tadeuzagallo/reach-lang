#pragma once

#include "Cell.h"
#include <iostream>

class Value {
    friend class Heap;

public:
    static Value crash();
    static Value unit();

    Value(); // defaults to Value(Crash)
    explicit Value(bool);
    explicit Value(double);
    explicit Value(Cell*);

    bool isUnit() const;
    bool isNumber() const;
    bool isBool() const;
    bool isCell() const;

    double asNumber() const;
    bool asBool() const;
    Cell* asCell() const;

    template<typename T>
    T* asCell() { return asCell()->cast<T>(); }

    void dump(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& out, const Value& value)
    {
        value.dump(out);
        return out;
    }

private:
    static constexpr int64_t TagTypeBool = 0b10;
    static constexpr int64_t TagTypeUnit = 0b100;
    static constexpr int64_t TagTypeNumber = 0xffff000000000000;
    static constexpr int64_t TagMask = TagTypeNumber | TagTypeUnit | TagTypeBool;
    static constexpr int64_t DoubleEncodeOffset = 0x1000000000000;

    enum CrashTag { Crash };
    enum UnitTag { Unit };

    explicit Value(CrashTag);
    explicit Value(UnitTag);

    bool isCrash() const;

    int64_t m_bits;
};
