#include "Value.h"

#include "Assert.h"
#include "Equality.h"
#include "Function.h"
#include "Type.h"
#include "VM.h"
#include <string>

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

Value::Value(uint32_t u)
    : Value(static_cast<double>(u))
{
}

Value::Value(int32_t i)
    : Value(static_cast<double>(i))
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

Value::Value(AbstractValue abstractValue)
{
    m_bits = abstractValue.bits() | TagTypeAbstractValue;
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
    CHECK();
    if (m_bits == other.m_bits)
        return true;
    if ((m_bits & TagMask) != (other.m_bits & TagMask))
        return false;
    if (isCell())
        return isEqual(asCell(), other.asCell());
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
    else if (isAbstractValue())
        asAbstractValue().dump(out);
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

bool Value::isAbstractValue() const
{
    CHECK();
    return (m_bits & TagMask) == TagTypeAbstractValue;
}

bool Value::isType() const
{
    return isCell() && asCell()->is<Type>();
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

AbstractValue Value::asAbstractValue() const
{
    Type* type = reinterpret_cast<Type*>(m_bits & ~TagTypeAbstractValue);
    return AbstractValue(type);
}

Type* Value::asType() const
{
    ASSERT(isType(), "OOPS");
    return asCell<Type>();
}

Type* Value::type(VM& vm) const
{
    if (isBool())
        return vm.boolType;
    if (isNumber())
        return vm.numberType;
    if (isUnit())
        return vm.unitType;
    if (isAbstractValue())
        return asAbstractValue().type();

    ASSERT(isCell(), "OOPS");
    Cell* cell = asCell();

    if (cell->is<Type>())
        return vm.typeType;

    if (cell->is<Typed>())
        return cell->cast<Typed>()->type();

    ASSERT_NOT_REACHED();
    return nullptr;
}

bool Value::isCrash() const
{
    return !m_bits;
}

Cell* Value::getCell() const
{
    ASSERT(isCell() || isAbstractValue(), "OOPS");
    return isCell() ? asCell() : asAbstractValue().type();
}

Value::SafeDump::SafeDump(Value value)
    : m_value(value)
{
}

void Value::SafeDump::dump(std::ostream& out) const
{
    if (m_value.isCrash())
        out << "<crash>";
    else
        out << m_value;
}
