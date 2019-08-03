#include "PartialEvaluator.h"

#include "Array.h"
#include "Environment.h"
#include "Hole.h"
#include "Object.h"
#include "Tuple.h"
#include "VM.h"

// Helpers

static Array* partiallyEvaluateArray(std::vector<Value>::iterator begin, std::vector<Value>::iterator end, VM& vm, Environment* env)
{
    Array* array_ = Array::create(vm, nullptr, end - begin);
    uint32_t i = 0;
    while (begin != end)
        array_->setIndex(i++, partiallyEvaluate(*begin++, vm, env));
    return array_;
}

// Structural objects

Tuple* partiallyEvaluate(Tuple* tuple, VM& vm, Environment* env)
{
    Array* items = partiallyEvaluateArray(tuple->begin(), tuple->end(), vm, env);
    return Tuple::create(vm, nullptr, items->size(), &*items->begin());
}

Object* partiallyEvaluate(Object* object, VM& vm, Environment* env)
{
    Object* object_ = Object::create(vm, object->type(), object->size());
    for (const auto& pair : *object) {
        Value second = partiallyEvaluate(pair.second, vm, env);
        object_->set(pair.first, second);
    }
    return object_;
}

Array* partiallyEvaluate(Array* array, VM& vm, Environment* env)
{
    return partiallyEvaluateArray(array->begin(), array->end(), vm, env);
}

Value partiallyEvaluate(Hole* hole, VM& vm, Environment* env)
{
    return hole->partiallyEvaluate(vm, env);
}

Type* partiallyEvaluate(Type* type, VM& vm, Environment* env)
{
#define EVAL(__class) \
    case Type::Class::__class: \
        return type->as<Type##__class>()->partiallyEvaluate(vm, env);

    switch (type->typeClass()) {
    case Type::Class::AnyType:
    case Type::Class::AnyValue:
        ASSERT_NOT_REACHED();
        return nullptr;
    case Type::Class::Type:
    case Type::Class::Top:
    case Type::Class::Bottom:
    case Type::Class::Name:
    case Type::Class::Var:
        return type;
    case Type::Class::Hole:
        // TODO: update signature
        return partiallyEvaluate(type->as<Hole>(), vm, env).asCell<Hole>();
    EVAL(Function)
    EVAL(Array)
    EVAL(Record)
    EVAL(Tuple)
    EVAL(Union)
    EVAL(Binding)
    }

#undef EVAL
}

// Value

Value partiallyEvaluate(Value value, VM& vm, Environment* env)
{
    if (!value.isCell())
        return value;

    Cell* cell = value.asCell();
    if (cell->is<Hole>())
        return partiallyEvaluate(cell->cast<Hole>(), vm, env);
    if (cell->is<Type>())
        return partiallyEvaluate(cell->cast<Type>(), vm, env);
    if (cell->is<Object>())
        return partiallyEvaluate(cell->cast<Object>(), vm, env);
    if (cell->is<Array>())
        return partiallyEvaluate(cell->cast<Array>(), vm, env);
    if (cell->is<Tuple>())
        return partiallyEvaluate(cell->cast<Tuple>(), vm, env);
    return value;
}

// Holes

Value HoleVariable::partiallyEvaluate(VM&, Environment* env)
{
    bool success;
    Value value = env->get(name()->str(), success);
    if (success && !value.isAbstractValue())
        return value;
    return this;
}

Value HoleCall::partiallyEvaluate(VM& vm, Environment* env)
{
    Value callee = ::partiallyEvaluate(this->callee(), vm, env);
    Array* arguments = ::partiallyEvaluateArray(this->arguments()->begin(), this->arguments()->end(), vm, env);
    return HoleCall::create(vm, callee, arguments);
}

Value HoleSubscript::partiallyEvaluate(VM& vm, Environment* env)
{
    Value target = ::partiallyEvaluate(this->target(), vm, env);
    Value index = ::partiallyEvaluate(this->index(), vm, env);
    return HoleSubscript::create(vm, target, index);
}

Value HoleMember::partiallyEvaluate(VM& vm, Environment* env)
{
    Value object = ::partiallyEvaluate(this->object(), vm, env);
    return HoleMember::create(vm, object, property());
}

// Types

TypeFunction* TypeFunction::partiallyEvaluate(VM& vm, Environment* env) const
{
    Array* params = ::partiallyEvaluate(this->params(), vm, env);
    Type* returnType = ::partiallyEvaluate(this->returnType(), vm, env);
    return TypeFunction::create(vm, params->size(), &*params->begin(), returnType, m_inferredParameters);
}

TypeArray* TypeArray::partiallyEvaluate(VM& vm, Environment* env) const
{
    Type* itemType = ::partiallyEvaluate(this->itemType(), vm, env);
    return TypeArray::create(vm, itemType);
}

TypeRecord* TypeRecord::partiallyEvaluate(VM& vm, Environment* env) const
{
    Object* fields = Object::create(vm, nullptr, 0);
    for (auto& field : *this) {
        fields->set(field.first, ::partiallyEvaluate(field.second, vm, env));
    }
    return TypeRecord::create(vm, fields);
}

TypeTuple* TypeTuple::partiallyEvaluate(VM& vm, Environment* env) const
{
    Array* types = itemsTypes();
    size_t size = types->size();
    TypeTuple* newTuple = TypeTuple::create(vm, size);
    Array* newTypes = newTuple->itemsTypes();
    for (uint32_t i = 0; i < size; i++) {
        auto type = types->getIndex(i);
        newTypes->setIndex(i, ::partiallyEvaluate(type, vm, env));
    }
    return newTuple;
}

TypeUnion* TypeUnion::partiallyEvaluate(VM& vm, Environment* env) const
{
    Type* lhs = ::partiallyEvaluate(this->lhs(), vm, env);
    Type* rhs = ::partiallyEvaluate(this->rhs(), vm, env);
    return TypeUnion::create(vm, lhs, rhs);
}

TypeBinding* TypeBinding::partiallyEvaluate(VM& vm, Environment* env) const
{
    Type* type = ::partiallyEvaluate(this->type(), vm, env);
    return TypeBinding::create(vm, name(), type);
}
