#pragma once

#include "AbstractValue.h"
#include "Cell.h"
#include <iostream>
#include <unordered_map>

class Type;
using Substitutions = std::unordered_map<uint32_t, Type*>;

class Value {
    friend class Heap;
    friend class JIT;
    friend class SafeDump;

public:
    static Value crash();
    static Value unit();

    Value(); // defaults to Value(Crash)
    Value(bool);
    Value(double);
    Value(uint32_t);
    Value(int32_t);
    Value(const Cell*);
    Value(AbstractValue);

    int64_t bits() const { return m_bits; }

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
    bool hasHole() const;
    Value substitute(VM&, const Substitutions&) const;

    template<typename T>
    T* asCell() const { return asCell()->cast<T>(); }

    template<typename T>
    bool isCell() const { return isCell() && asCell()->is<T>(); }

    bool operator!=(const Value&) const;
    bool operator==(const Value&) const;
    void dump(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& out, const Value& value)
    {
        value.dump(out);
        return out;
    }

    class SafeDump;

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

class Value::SafeDump {
public:
    explicit SafeDump(Value);
    void dump(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& out, const SafeDump& value)
    {
        value.dump(out);
        return out;
    }

private:
    Value m_value;
};
