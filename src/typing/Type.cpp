#include "Type.h"

#include "TypeChecker.h"
#include <typeinfo>

static bool isEqualType(Value lhs, Value rhs)
{
    if (lhs.isType() != rhs.isType())
        return false;
    if (lhs.isType())
        return *lhs.asType() == *rhs.asType();
    return lhs == rhs;
}

Type::Type()
    : Object(0)
{
}

bool Type::operator!=(const Type& other) const
{
    return !(*this == other);
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

Type* TypeType::substitute(VM&, Substitutions&)
{
    return this;
}

bool TypeType::operator==(const Type& other) const
{
    return other.is<TypeType>();
}

void TypeType::dump(std::ostream& out) const
{
    out << "Type";
}

Type* TypeBottom::substitute(VM&, Substitutions&)
{
    return this;
}

bool TypeBottom::operator==(const Type& other) const
{
    return other.is<TypeBottom>();
}

void TypeBottom::dump(std::ostream& out) const
{
    out << "⊥";
}

TypeName::TypeName(const std::string& name)
{
    set_name(String::create(vm(), name));
}

Type* TypeName::substitute(VM&, Substitutions&)
{
    return this;
}

bool TypeName::operator==(const Type& other) const
{
    if (!other.is<TypeName>())
        return false;
    return name() == other.as<TypeName>()->name();
}

void TypeName::dump(std::ostream& out) const
{
    out << name()->str();
}

TypeFunction::TypeFunction(const Types& params, Value returnType)
{
    set_params(Array::create(vm(), params));
    set_returnType(returnType);

    Types implicitParams;
    Types explicitParams;
    for (Value param : params) {
        if (param.isType()) {
            Type* paramType = param.asType();
            if (paramType->is<TypeVar>() && paramType->as<TypeVar>()->inferred()) {
                implicitParams.emplace_back(param);
                continue;
            }
        }

        explicitParams.emplace_back(param);
    }
    set_explicitParamCount(explicitParams.size());
    set_implicitParamCount(implicitParams.size());
    set_explicitParams(Array::create(vm(), explicitParams));
    set_implicitParams(Array::create(vm(), implicitParams));
}

size_t TypeFunction::paramCount() const
{
    return params()->size();
}

Type* TypeFunction::param(uint32_t index) const
{
    ASSERT(index < paramCount(), "Out of bounds access to TypeFunction::param");
    return params()->getIndex(index).asCell<Type>();
}

TypeVar* TypeFunction::implicitParam(uint32_t index) const
{
    ASSERT(index < implicitParamCount(), "Out of bounds access to TypeFunction::implicitParam");
    return implicitParams()->getIndex(index).asCell<TypeVar>();
}

Type* TypeFunction::instantiate(VM& vm)
{
    Substitutions subst;
    for (Value v : *params()) {
        if (v.isType()) {
            Type* type = v.asType();
            if (type->is<TypeVar>())
                type->as<TypeVar>()->fresh(vm, subst);
        }
    }
    return substitute(vm, subst);
}

Type* TypeFunction::substitute(VM& vm, Substitutions& subst)
{
    Types params_;
    for (Value v : *params()) {
        if (v.isType())
            params_.emplace_back(v.asType()->substitute(vm, subst));
        else
            params_.emplace_back(v);
    }
    Value returnType_ = returnType();
    if (returnType_.isType())
        returnType_ = returnType_.asType()->substitute(vm, subst);
    return TypeFunction::create(vm, params_, returnType_);
}

bool TypeFunction::operator==(const Type& otherT) const
{
    if (!otherT.is<TypeFunction>())
        return false;
    const TypeFunction* other = otherT.as<TypeFunction>();
    if (params()->size() != other->params()->size())
        return false;
    for (unsigned i = 0; i < params()->size(); i++)
        if (isEqualType(param(i), other->param(i)))
            return false;
    if (isEqualType(returnType(), other->returnType()))
        return false;
    return true;
}

void TypeFunction::dump(std::ostream& out) const
{
    out << "(";
    bool isFirst = true;
    for (Value param : *params()) {
        if (!isFirst)
            out << ", ";
        out << param;
        isFirst = false;
    }
    out << ") -> " << returnType();
}

TypeArray::TypeArray(Value itemType)
{
    set_itemType(itemType);
}

Type* TypeArray::substitute(VM& vm, Substitutions& subst)
{
    Value itemType_ = itemType();
    if (itemType_.isType())
        itemType_ = itemType_.asType()->substitute(vm, subst);
    return TypeArray::create(vm, itemType_);
}

bool TypeArray::operator==(const Type& other) const
{
    if (!other.is<TypeArray>())
        return false;
    return isEqualType(itemType(), other.as<TypeArray>()->itemType());
}

void TypeArray::dump(std::ostream& out) const
{
    out << *itemType().type(vm()) << "[]";
}

TypeRecord::TypeRecord(const Fields& fields)
{
    set_fields(Object::create(vm(), fields));
}

Type* TypeRecord::field(const std::string& name) const
{
    std::optional<Value> field = fields()->tryGet(name);
    if (!field)
        return nullptr;
    return field->asCell<Type>();
}

Type* TypeRecord::substitute(VM& vm, Substitutions& subst)
{
    Fields fields_;
    for (auto& field : *fields())
        fields_.emplace(field.first, field.second.asCell<Type>()->substitute(vm, subst));
    return TypeRecord::create(vm, fields_);
}

bool TypeRecord::operator==(const Type& otherT) const
{
    if (!otherT.is<TypeRecord>())
        return false;
    const TypeRecord* other = otherT.as<TypeRecord>();
    if (fields()->size() != other->fields()->size())
        return false;
    for (const auto& pair : *fields()) {
        auto otherField = other->field(pair.first);
        if (!otherField || pair.second != otherField)
            return false;
    }
    return true;
}

void TypeRecord::dump(std::ostream& out) const
{
    out << "{";
    bool isFirst = true;
    for (auto& pair : *fields()) {
        if (!isFirst)
            out << ", ";
        out << pair.first << ": " << *pair.second.type(vm());
        isFirst = false;
    }
    out << "}";
}

uint32_t TypeVar::s_uid = 0;

TypeVar::TypeVar(const std::string& name, bool inferred, bool rigid)
    : m_isRigid(rigid)
{
    set_uid(++s_uid);
    set_inferred(inferred);
    set_name(String::create(vm(), name));
}

void TypeVar::fresh(VM& vm, Substitutions& subst) const
{
    TypeVar* newVar = TypeVar::create(vm, name()->str(), inferred(), false);
    subst.emplace(uid(), newVar);
}

Type* TypeVar::substitute(VM&, Substitutions& subst)
{
    const auto it = subst.find(uid());
    if (it != subst.end())
        return it->second;
    return this;
}

bool TypeVar::operator==(const Type& other) const
{
    if (!other.is<TypeVar>())
        return false;
    return uid() == other.as<TypeVar>()->uid();
}

void TypeVar::dump(std::ostream& out) const
{
    out << name()->str();
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
    }
    return out;
}
