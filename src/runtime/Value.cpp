#include "Value.h"

#include "Assert.h"

#define CHECK() ASSERT(!isCrash(), "Value::crash")

Value Value::crash()
{
  return Value { Crash };
}

Value Value::unit()
{
    return Value { Unit };
}

Value::Value()
    : Value(Crash)
{
}

Value::Value(bool b)
    : m_bits(TagTypeBool | b)
{
}

Value::Value(double d)
{
    union {
        double d;
        int64_t i;
    } u { d };
    m_bits = u.i + DoubleEncodeOffset;
}

Value::Value(const Cell* cell)
{
    m_bits = reinterpret_cast<intptr_t>(cell);
}

Value::Value(CrashTag)
    : m_bits(0)
{
}

Value::Value(UnitTag)
    : m_bits(TagTypeUnit)
{
}

bool Value::operator!=(const Value& other) const
{
    return !(*this == other);
}

bool Value::operator==(const Value& other) const
{
    if (m_bits == other.m_bits)
        return true;
    // TODO: proper equalify for cells
    return false;
}

void Value::dump(std::ostream& out) const
{
    if (isBool())
        out << (asBool() ? "true" : "false");
    else if (isNumber())
        out << asNumber();
    else if (isUnit())
        out << "()";
    else if (isCell())
        asCell()->dump(out);
}

bool Value::isUnit() const
{
    CHECK();
    return m_bits == TagTypeUnit;
}

bool Value::isNumber() const
{
    CHECK();
    return m_bits & TagTypeNumber;
}

bool Value::isBool() const
{
    CHECK();
    return (m_bits & ~1) == TagTypeBool;
}

bool Value::isCell() const
{
    CHECK();
    return !(m_bits & TagMask);
}

double Value::asNumber() const
{
    ASSERT(isNumber(), "Value is not a number");
    union {
        int64_t i;
        double d;
    } u { m_bits - DoubleEncodeOffset };
    return u.d;
}

bool Value::asBool() const
{
    ASSERT(isBool(), "Value is not a bool");
    return m_bits & 1;
}

Cell* Value::asCell() const
{
    ASSERT(isCell(), "Value is not a cell");
    return reinterpret_cast<Cell*>(m_bits);
}

bool Value::isCrash() const
{
    return !m_bits;

}
