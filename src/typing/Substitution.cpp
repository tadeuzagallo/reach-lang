#include "Hole.h"
#include "Tuple.h"
#include "Type.h"

Value Value::substitute(VM& vm, const Substitutions& subst) const
{
#define SUBSTITUTE(__kind) \
    case Cell::Kind::__kind: \
        return cell->cast<__kind>()->substitute(vm, subst);

    if (isAbstractValue())
        return asAbstractValue().type()->substitute(vm, subst);
    if (!isCell())
        return *this;
    Cell* cell = asCell();
    switch (cell->kind()) {
    SUBSTITUTE(Hole)
    SUBSTITUTE(Type)
    SUBSTITUTE(Object)
    SUBSTITUTE(Array)
    SUBSTITUTE(Tuple)
    default:
        return *this;
    }

#undef SUBSTITUTE
}

// Object

Object* Object::substitute(VM& vm, const Substitutions& subst) const
{
    Object* object_ = Object::create(vm, type(), size());
    for (const auto& pair : *this) {
        Value second = pair.second.substitute(vm, subst);
        object_->set(pair.first, second);
    }
    return object_;
}

// Array

Array* Array::substitute(VM& vm, const Substitutions& subst) const
{
    Array* array = Array::create(vm, type(), size());
    uint32_t i = 0;
    for (const auto& item : *this)
        array->setIndex(i++, item.substitute(vm, subst));
    return array;
}

// Tuple

Tuple* Tuple::substitute(VM& vm, const Substitutions& subst) const
{
    Tuple* tuple = Tuple::create(vm, type(), size());
    uint32_t i = 0;
    for (const auto& item : *this)
        tuple->setIndex(i++, item.substitute(vm, subst));
    return tuple;
}

// Types

Type* TypeType::substitute(VM&, const Substitutions&) const
{
    return const_cast<TypeType*>(this);
}

Type* TypeTop::substitute(VM&, const Substitutions&) const
{
    return const_cast<TypeTop*>(this);
}

Type* TypeBottom::substitute(VM&, const Substitutions&) const
{
    return const_cast<TypeBottom*>(this);
}

Type* TypeName::substitute(VM&, const Substitutions&) const
{
    return const_cast<TypeName*>(this);
}

Type* TypeFunction::substitute(VM& vm, const Substitutions& subst) const
{
    Array* params = this->params()->substitute(vm, subst);
    Value returnType = this->returnType().substitute(vm, subst);
    return TypeFunction::create(vm, params->size(), &*params->begin(), returnType, m_inferredParameters);
}

Type* TypeArray::substitute(VM& vm, const Substitutions& subst) const
{
    Value itemType = this->itemType().substitute(vm, subst);
    return TypeArray::create(vm, itemType);
}

Type* TypeTuple::substitute(VM& vm, const Substitutions& subst) const
{
    Array* types = itemsTypes();
    size_t size = types->size();
    TypeTuple* newTuple = TypeTuple::create(vm, size);
    Array* newTypes = newTuple->itemsTypes();
    for (uint32_t i = 0; i < size; i++) {
        auto type = types->getIndex(i);
        newTypes->setIndex(i, type.substitute(vm, subst));
    }
    return newTuple;
}

Type* TypeRecord::substitute(VM& vm, const Substitutions& subst) const
{
    Fields fields;
    for (auto& field : *this) {
        fields.emplace(field.first, field.second.substitute(vm, subst));
    }
    return TypeRecord::create(vm, fields);
}

Type* TypeVar::substitute(VM&, const Substitutions& subst) const
{
    const auto it = subst.find(uid());
    if (it != subst.end())
        return it->second;
    return const_cast<TypeVar*>(this);
}

Type* TypeUnion::substitute(VM& vm, const Substitutions& subst) const
{
    Value lhs = this->lhs().substitute(vm, subst);
    Value rhs = this->rhs().substitute(vm, subst);
    return TypeUnion::create(vm, lhs, rhs);
}

Type* TypeBinding::substitute(VM& vm, const Substitutions& substitutions) const
{
    return TypeBinding::create(vm, name(), type()->substitute(vm, substitutions));
}

// Holes

Hole* HoleVariable::substitute(VM&, const Substitutions&) const
{
    return const_cast<HoleVariable*>(this);
}


Hole* HoleCall::substitute(VM& vm, const Substitutions& subst) const
{
    Value callee = this->callee().substitute(vm, subst);
    Array* arguments = this->arguments()->substitute(vm, subst);
    return HoleCall::create(vm, callee, arguments);
}

Hole* HoleSubscript::substitute(VM& vm, const Substitutions& subst) const
{
    Value target = this->target().substitute(vm, subst);
    Value index = this->index().substitute(vm, subst);
    return HoleSubscript::create(vm, target, index);
}

Hole* HoleMember::substitute(VM& vm, const Substitutions& subst) const
{
    Value object = this->object().substitute(vm, subst);
    return HoleMember::create(vm, object, property());
}
