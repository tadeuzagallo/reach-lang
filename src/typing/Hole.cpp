#include "Hole.h"

#include "AST.h"
#include "Array.h"
#include "Environment.h"
#include "Function.h"
#include "Object.h"
#include "Tuple.h"

Hole::Hole()
    : Type(Type::Class::Hole)
{
}

bool Hole::operator!=(const Hole& other) const
{
    return !(*this == other);
}

HoleVariable::HoleVariable(VM& vm, const std::string& identifier)
{
    set_name(String::create(vm, identifier));
}

bool HoleVariable::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleVariable*>(&other))
        return *name() == *hole->name();
    return false;
}

void HoleVariable::dump(std::ostream& out) const
{
    out << name()->str();
}

HoleCall::HoleCall(VM&, Value callee, Array* arguments)
{
    set_callee(callee);
    set_arguments(arguments);
}

bool HoleCall::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleCall*>(&other)) {
        if (callee() != hole->callee())
            return false;
        size_t size = arguments()->size();
        if (size != hole->arguments()->size())
            return false;
        for (uint32_t i = 0; i < size; i++) {
            if (arguments()->getIndex(i) != hole->arguments()->getIndex(i))
                return false;
        }
        return true;
    }
    return false;
}

void HoleCall::dump(std::ostream& out) const
{
    Value callee = this->callee();
    if (callee.isCell<Function>())
        out << callee.asCell<Function>()->name();
    else
        callee.dump(out);
    out << "(";
    bool isFirst = true;
    for (const auto& argument : *arguments()) {
        if (!isFirst)
            out << ", ";
        isFirst = false;
        argument.dump(out);
    }
    out << ")";

}

HoleSubscript::HoleSubscript(VM&, Value target, Value index)
{
    set_target(target);
    set_index(index);
}

bool HoleSubscript::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleSubscript*>(&other)) {
        if (target() != hole->target())
            return false;
        return index() == hole->index();
    }
    return false;
}

void HoleSubscript::dump(std::ostream& out) const
{
    target().dump(out);
    out << "[";
    index().dump(out);
    out << "]";
}

HoleMember::HoleMember(VM&, Value object, String* property)
{
    set_object(object);
    set_property(property);
}

bool HoleMember::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleMember*>(&other)) {
        if (object() != hole->object())
            return false;
        return *property() == *hole->property();
    }
    return false;
}

void HoleMember::dump(std::ostream& out) const
{
    out << object() << "." << property()->str();
}

// Value::hasHole
template<typename T>
bool hasHole(T);

template<>
bool hasHole(Tuple* tuple)
{
    for (const auto& value : *tuple)
        if (value.hasHole())
            return true;
    return false;
}

bool hasHole(Object* object)
{
    for (const auto& pair : *object)
        if (pair.second.hasHole())
            return true;
    return false;
}

bool hasHole(Array* array)
{
    for (const auto& value : *array)
        if (value.hasHole())
            return true;
    return false;
}

bool Value::hasHole() const
{
    if (!isCell())
        return false;

    Cell* cell = asCell();
    if (cell->is<Hole>())
        return true;
    if (cell->is<Object>())
        return ::hasHole(cell->cast<Object>());
    if (cell->is<Array>())
        return ::hasHole(cell->cast<Array>());
    if (cell->is<Tuple>())
        return ::hasHole(cell->cast<Tuple>());
    return false;
}
