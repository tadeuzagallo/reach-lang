#include "Equality.h"

#include "Cell.h"
#include "Hole.h"
#include "Type.h"

template<>
bool isEqual(Cell* lhs, Cell* rhs)
{
#define EQUALITY(__type) \
    case Cell::Kind::__type: \
        return *lhs->cast<__type>() == *rhs->cast<__type>();

    if (lhs->kind() != rhs->kind())
        return false;

    switch (lhs->kind()) {
    EQUALITY(Hole)
    EQUALITY(Type)
    EQUALITY(Object)
    EQUALITY(Array)
    //EQUALITY(Tuple)
    default:
        return false;
    }

#undef EQUALITY
}

// Array

bool Array::operator==(const Array& other) const
{
    if (size() != other.size())
        return false;
    for (uint32_t i = 0; i < size(); i++)
        if (getIndex(i) != other.getIndex(i))
            return false;
    return true;
}

// Object

bool Object::operator==(const Object& other) const
{
    if (size() != other.size())
        return false;
    for (const auto& field : *this) {
        if (auto value = other.tryGet(field.first)) {
            if (field.second == *value)
                continue;
        }
        return false;
    }
    return true;
}

// Types

bool Type::operator==(const Type& other) const
{
#define EQUALITY(__type) \
    case Type::Class::__type: \
        return as<Type##__type>()->isEqual(other.as<Type##__type>());

    if (typeClass() != other.typeClass())
        return false;

    switch (typeClass()) {
    case Type::Class::AnyValue:
    case Type::Class::AnyType:
        ASSERT_NOT_REACHED();
        return false;
    case Type::Class::Type:
    case Type::Class::Top:
    case Type::Class::Bottom:
        return true;
    case Type::Class::Hole:
        return *this == *other.as<Hole>();
    EQUALITY(Name)
    EQUALITY(Function)
    EQUALITY(Array)
    EQUALITY(Record)
    EQUALITY(Var)
    EQUALITY(Tuple)
    EQUALITY(Union)
    EQUALITY(Binding)
    }

#undef EQUALITY
}

bool TypeName::isEqual(const TypeName* other) const
{
    // The names should be the same, but they should also be unique
    return this == other;
}

bool TypeFunction::isEqual(const TypeFunction* other) const
{
    return *params() == *other->params() && returnType() == other->returnType();
}

bool TypeArray::isEqual(const TypeArray* other) const
{
    return itemType() == other->itemType();
}

bool TypeRecord::isEqual(const TypeRecord* other) const
{
    return this->Object::operator==(*other);
}

bool TypeVar::isEqual(const TypeVar* other) const
{
    return uid() == other->uid();
}

bool TypeTuple::isEqual(const TypeTuple* other) const
{
    return *itemsTypes() == *other->itemsTypes();
}

bool TypeUnion::isEqual(const TypeUnion* other) const
{
    return lhs() == other->lhs() && rhs() == other->rhs();
}

bool TypeBinding::isEqual(const TypeBinding* other) const
{
    return *type() == *other->type();
}
