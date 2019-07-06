#include "AST.h"
#include "Hole.h"

Hole::Hole()
    : Object(0)
{
}

bool Hole::operator!=(const Hole& other) const
{
    return !(*this == other);
}

Hole* ParenthesizedExpression::asHole(VM& vm)
{
    return expression->asHole(vm);
}

Hole* Identifier::asHole(VM& vm)
{
    return HoleIdentifier::create(vm, *this);
}

HoleIdentifier::HoleIdentifier(VM& vm, const Identifier& identifier)
{
    set_name(String::create(vm, identifier.name));
}

bool HoleIdentifier::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleIdentifier*>(&other))
        return *name() == *hole->name();
    return false;
}

void HoleIdentifier::dump(std::ostream& out) const
{
    out << name()->str();
}

Hole* TupleExpression::asHole(VM& vm)
{
    return HoleTuple::create(vm, *this);
}

HoleTuple::HoleTuple(VM& vm, const TupleExpression& tuple)
{

    size_t size = tuple.items.size();
    Array* items = Array::create(vm, size);
    for (uint32_t i = 0; i < size; i++)
        items->setIndex(i, tuple.items[i]->asHole(vm));
    set_items(items);
}

bool HoleTuple::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleTuple*>(&other)) {
        size_t size = items()->size();
        if (size != hole->items()->size())
            return false;
        for (uint32_t i = 0; i < size; i++) {
            if (*items()->getIndex(i).asCell<Hole>() != *hole->items()->getIndex(i).asCell<Hole>())
                return false;
        }
        return true;
    }

    return false;
}

void HoleTuple::dump(std::ostream& out) const
{
    out << "(";
    bool isFirst = true;
    for (const auto& item : *items()) {
        if (!isFirst)
            out << ", ";
        isFirst = false;
        item.dump(out);
    }
    out << ")";
}

Hole* ObjectLiteralExpression::asHole(VM& vm)
{
    return HoleObject::create(vm, *this);
}

HoleObject::HoleObject(VM& vm,  const ObjectLiteralExpression& object)
{
    for (const auto& pair : object.fields)
        set(pair.first->name, pair.second->asHole(vm));
}

bool HoleObject::operator==(const Hole& other) const
{
    if (const auto& hole = dynamic_cast<const HoleObject*>(&other)) {
        if (size() != hole->size())
            return false;
        for (const auto& pair : *this) {
            if (auto value = hole->tryGet(pair.first)) {
                if (*pair.second.asCell<Hole>() != *value->asCell<Hole>())
                    return false;
            }
        }
        return true;
    }
    return false;
}

void HoleObject::dump(std::ostream& out) const
{
    Object::dump(out);
}

Hole* ArrayLiteralExpression::asHole(VM& vm)
{
    return HoleArray::create(vm, *this);
}

HoleArray::HoleArray(VM& vm, const ArrayLiteralExpression& array)
{
    size_t size = array.items.size();
    Array* items = Array::create(vm, size);
    for (uint32_t i = 0; i < size; i++)
        items->setIndex(i, array.items[i]->asHole(vm));
    set_items(items);
}

bool HoleArray::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleArray*>(&other)) {
        size_t size = items()->size();
        if (size != hole->items()->size())
            return false;
        for (uint32_t i = 0; i < size; i++) {
            if (*items()->getIndex(i).asCell<Hole>() != *hole->items()->getIndex(i).asCell<Hole>())
                return false;
        }
        return true;
    }
    return false;
}

void HoleArray::dump(std::ostream& out) const
{
    items()->dump(out);
}

Hole* CallExpression::asHole(VM& vm)
{
    return HoleCall::create(vm, *this);
}

HoleCall::HoleCall(VM& vm, const CallExpression& call)
{
    set_callee(call.callee->asHole(vm));

    size_t size = call.arguments.size();
    Array* arguments = Array::create(vm, size);
    for (uint32_t i = 0; i < size; i++)
        arguments->setIndex(i, call.arguments[i]->asHole(vm));
    set_arguments(arguments);
}

bool HoleCall::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleCall*>(&other)) {
        if (*callee() != *hole->callee())
            return false;
        size_t size = arguments()->size();
        if (size != hole->arguments()->size())
            return false;
        for (uint32_t i = 0; i < size; i++) {
            if (*arguments()->getIndex(i).asCell<Hole>() != *hole->arguments()->getIndex(i).asCell<Hole>())
                return false;
        }
        return true;
    }
    return false;
}

void HoleCall::dump(std::ostream& out) const
{
    callee()->dump(out);
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

Hole* SubscriptExpression::asHole(VM& vm)
{
    return HoleSubscript::create(vm, *this);
}

HoleSubscript::HoleSubscript(VM& vm, const SubscriptExpression& subscript)
{
    set_target(subscript.target->asHole(vm));
    set_index(subscript.index->asHole(vm));
}

bool HoleSubscript::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleSubscript*>(&other)) {
        if (*target() != *hole->target())
            return false;
        return *index() == *hole->index();
    }
    return false;
}

void HoleSubscript::dump(std::ostream& out) const
{
    target()->dump(out);
    out << "[";
    index()->dump(out);
    out << "]";
}

Hole* MemberExpression::asHole(VM& vm)
{
    return HoleMember::create(vm, *this);
}

HoleMember::HoleMember(VM& vm, const MemberExpression& member)
{
    set_object(member.object->asHole(vm));
    set_property(String::create(vm, member.property->name));
}

bool HoleMember::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleMember*>(&other)) {
        if (*object() != *hole->object())
            return false;
        return *property() == *hole->property();
    }
    return false;
}

void HoleMember::dump(std::ostream& out) const
{
    object()->dump(out);
    out << ".";
    property()->dump(out);
}

Hole* LiteralExpression::asHole(VM& vm)
{
    return HoleLiteral::create(vm, *this);
}

Value BooleanLiteral::asValue(VM&)
{
    return value;
}

Value NumericLiteral::asValue(VM&)
{
    return value;
}

Value StringLiteral::asValue(VM& vm)
{
    return String::create(vm, value);
}

HoleLiteral::HoleLiteral(VM& vm, const LiteralExpression& literal)
{
    set_value(literal.literal->asValue(vm));
}

bool HoleLiteral::operator==(const Hole& other) const
{
    if (const auto* hole = dynamic_cast<const HoleLiteral*>(&other))
        return value() == hole->value();
    return false;
}

void HoleLiteral::dump(std::ostream& out) const
{
    out << value();
}

Hole* TypeExpression::asHole(VM&)
{
    ASSERT_NOT_REACHED();
    return nullptr;
}
