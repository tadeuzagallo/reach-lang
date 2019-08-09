#include "Type.h"

#include "BytecodeBlock.h"
#include "TypeChecker.h"
#include <typeinfo>

Type::Type(Class typeClass)
    : Object(nullptr, 0)
    , m_class(typeClass)
{
    ASSERT(typeClass >= Class::SpecificType, "OOPS");
}

Type* Type::instantiate(VM&)
{
    return this;
}

std::ostream& operator<<(std::ostream& out, const Type& type)
{
    type.dump(out);
    return out;
}

TypeType::TypeType()
    : Type(Type::Class::Type)
{
}

void TypeType::dump(std::ostream& out) const
{
    out << "Type";
}

TypeTop::TypeTop()
    : Type(Type::Class::Top)
{
}

void TypeTop::dump(std::ostream& out) const
{
    out << "⊤";
}

TypeBottom::TypeBottom()
    : Type(Type::Class::Bottom)
{
}

void TypeBottom::dump(std::ostream& out) const
{
    out << "⊥";
}

TypeName::TypeName(const std::string& name)
    : Type(Type::Class::Name)
{
    set_name(String::create(vm(), name));
}

void TypeName::dump(std::ostream& out) const
{
    out << name()->str();
}

TypeFunction::TypeFunction(uint32_t parameterCount, const Value* parameters, Type* returnType, uint32_t inferredParameters)
    : Type(Type::Class::Function)
    , m_inferredParameters(inferredParameters)
{
    CellArray<Type>* params = CellArray<Type>::create(vm(), parameterCount, parameters);
    set_params(params);
    set_returnType(returnType);

    uint32_t implicitParamCount = 0;
    uint32_t explicitParamCount = 0;
    for (uint32_t i = 0; i < parameterCount; i++) {
        if (inferredParameters & (1 << i))
            ++implicitParamCount;
        else
            ++explicitParamCount;
    }
    set_explicitParamCount(explicitParamCount);
    set_implicitParamCount(implicitParamCount);

    CellArray<Type>* implicitParams = CellArray<Type>::create(vm(), implicitParamCount);
    CellArray<Type>* explicitParams = CellArray<Type>::create(vm(), explicitParamCount);
    while (parameterCount--) {
        Type* param = params->getIndex(parameterCount);
        if (inferredParameters & (1 << parameterCount))
            implicitParams->setIndex(--implicitParamCount, param);
        else
            explicitParams->setIndex(--explicitParamCount, param);
    }
    set_explicitParams(explicitParams);
    set_implicitParams(implicitParams);
}

size_t TypeFunction::paramCount() const
{
    return params()->size();
}

Type* TypeFunction::param(uint32_t index) const
{
    ASSERT(index < paramCount(), "Out of bounds access to TypeFunction::param");
    return params()->getIndex(index);
}

Type* TypeFunction::implicitParam(uint32_t index) const
{
    ASSERT(index < implicitParamCount(), "Out of bounds access to TypeFunction::implicitParam");
    return implicitParams()->getIndex(index);
}

Type* TypeFunction::instantiate(VM& vm)
{
    Substitutions subst;
    for (Type* type : *params()) {
        if (type->is<TypeBinding>())
            type = type->as<TypeBinding>()->type();
        if (type->is<TypeVar>())
            type->as<TypeVar>()->fresh(vm, subst);
    }
    return substitute(vm, subst);
}

void TypeFunction::dump(std::ostream& out) const
{
    out << "(";
    bool isFirst = true;
    for (Type* param : *params()) {
        if (!isFirst)
            out << ", ";
        isFirst = false;
        if (param->is<TypeBinding>())
            param->as<TypeBinding>()->fullDump(out);
        else
            out << *param;
    }
    out << ") -> " << *returnType();
}

TypeArray::TypeArray(Type* itemType)
    : Type(Type::Class::Array)
{
    set_itemType(itemType);
}

void TypeArray::dump(std::ostream& out) const
{
    out << *itemType() << "[]";
}

TypeTuple::TypeTuple(uint32_t itemCount)
    : Type(Type::Class::Tuple)
{
    set_itemsTypes(CellArray<Type>::create(vm(), itemCount));
}

void TypeTuple::dump(std::ostream& out) const
{
    out << "<";
    bool first = true;
    for (const auto& type : *itemsTypes()) {
        if (!first)
            out << ", ";
        first = false;
        out << *type;
    }
    out << ">";
}

TypeRecord::TypeRecord(Object* fields)
    : Type(Type::Class::Record)
{
    *static_cast<Object*>(this) = *fields;
}

TypeRecord::TypeRecord(const BytecodeBlock& block, uint32_t fieldCount, const Value* keys, const Value* types)
    : Type(Type::Class::Record)
{
    for (uint32_t i = 0; i < fieldCount; i++) {
        const std::string& key = block.identifier(keys[i].asNumber());
        set(key, types[i]);
    }
}

Type* TypeRecord::field(const std::string& name) const
{
    std::optional<Value> field = tryGet(name);
    if (!field)
        return nullptr;
    return field->asCell<Type>();
}

void TypeRecord::dump(std::ostream& out) const
{
    out << "{";
    bool isFirst = true;
    for (auto& pair : *this) {
        if (!isFirst)
            out << ", ";
        out << pair.first << ": " << pair.second;
        isFirst = false;
    }
    if (isFirst)
        out << ":";
    out << "}";
}

uint32_t TypeVar::s_uid = 0;

TypeVar::TypeVar(const std::string& name, bool inferred, bool rigid, Type* bounds)
    : Type(Type::Class::Var)
    , m_isRigid(rigid)
{
    set_uid(++s_uid);
    set_inferred(inferred);
    set_name(String::create(vm(), name));
    set_bounds(bounds);
}

void TypeVar::fresh(VM& vm, Substitutions& subst) const
{
    TypeVar* newVar = TypeVar::create(vm, name()->str(), inferred(), false, bounds());
    subst.emplace(uid(), newVar);
}

void TypeVar::dump(std::ostream& out) const
{
    out << name()->str();
}

TypeUnion::TypeUnion(Type* lhs, Type* rhs)
    : Type(Type::Class::Union)
{
    set_lhs(lhs);
    set_rhs(rhs);
}

Type* TypeUnion::collapse(VM& vm)
{
    static auto getFields = [&](Value value) -> Object* {
        ASSERT(value.isType(), "OOPS");

        Type* type = value.asType();

        while (type->is<TypeUnion>())
            type = type->as<TypeUnion>()->collapse(vm);

        if (!type->is<TypeRecord>())
            return nullptr;

        return type->as<TypeRecord>();
    };

    Object* lhs = getFields(this->lhs());
    if (!lhs)
        return nullptr;
    Object* rhs = getFields(this->rhs());
    if (!rhs)
        return nullptr;

    Object* fields = Object::create(vm, nullptr, 0);
    for (const auto& lhsField : *lhs) {
        if (auto rhsValue = rhs->tryGet(lhsField.first))
            fields->set(lhsField.first, TypeUnion::create(vm, lhsField.second.asType(), rhsValue->asType()));
    }
    return TypeRecord::create(vm, fields);
}

void TypeUnion::dump(std::ostream& out) const
{
    out << *lhs() << " | " << *rhs();
}

TypeBinding::TypeBinding(String* name, Type* type)
    : Type(Type::Class::Binding)
{
    set_name(name);
    set_type(type);
}

void TypeBinding::dump(std::ostream& out) const
{
    type()->dump(out);
}

void TypeBinding::fullDump(std::ostream& out) const
{
    out << name()->str() << ": ";
    Type* type = this->type();
    if (type->is<TypeVar>() && *type->as<TypeVar>()->name() == *name())
        out << "Type";
    else
        type->dump(out);
}

std::ostream& operator<<(std::ostream& out, Type::Class tc)
{
    switch (tc) {
    case Type::Class::AnyValue:
        out << "Type::Class::AnyValue";
        break;
    case Type::Class::AnyType:
        out << "Type::Class::AnyType";
        break;
    case Type::Class::Type:
        out << "Type::Class::Type";
        break;
    case Type::Class::Top:
        out << "Type::Class::Top";
        break;
    case Type::Class::Bottom:
        out << "Type::Class::Bottom";
        break;
    case Type::Class::Name:
        out << "Type::Class::Name";
        break;
    case Type::Class::Function:
        out << "Type::Class::Function";
        break;
    case Type::Class::Array:
        out << "Type::Class::Array";
        break;
    case Type::Class::Record:
        out << "Type::Class::Record";
        break;
    case Type::Class::Var:
        out << "Type::Class::Var";
        break;
    case Type::Class::Tuple:
        out << "Type::Class::Tuple";
        break;
    case Type::Class::Union:
        out << "Type::Class::Union";
        break;
    case Type::Class::Hole:
        out << "Type::Class::Hole";
        break;
    case Type::Class::Binding:
        out << "Type::Class::Binding";
        break;
    }
    return out;
}

// JIT helpers
TypeVar* createTypeVar(VM& vm, const std::string& name, bool isInferred, bool isRigid, Type* bounds)
{
    return TypeVar::create(vm, name, isInferred, isRigid, bounds);
}

TypeName* createTypeName(VM& vm, const std::string& name)
{
    return TypeName::create(vm, name);
}

TypeArray* createTypeArray(VM& vm, Type* itemType)
{
    return TypeArray::create(vm, itemType);
}

TypeTuple* createTypeTuple(VM& vm, uint32_t itemCount)
{
    return TypeTuple::create(vm, itemCount);
}

TypeRecord* createTypeRecord(VM& vm, const BytecodeBlock& block, uint32_t fieldCount, const Value* keys, const Value* types)
{
    return TypeRecord::create(vm, block, fieldCount, keys, types);
}

TypeFunction* createTypeFunction(VM& vm, uint32_t paramCount, const Value* params, Type* returnType, uint32_t inferredParameters)
{
    return TypeFunction::create(vm, paramCount, params, returnType, inferredParameters);
}

TypeUnion* createTypeUnion(VM& vm, Type* lhs, Type* rhs)
{
    return TypeUnion::create(vm, lhs, rhs);
}

TypeBinding* createTypeBinding(VM& vm, const std::string& name, Type* type)
{
    return TypeBinding::create(vm, String::create(vm, name), type);
}
