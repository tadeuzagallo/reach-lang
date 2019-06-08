#pragma once

#include "Array.h"
#include "Object.h"
#include "RhString.h"
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>

class Type;

using Types = std::vector<Type*>;
using Fields = std::unordered_map<std::string, Type*>;
using Substitutions = std::unordered_map<uint32_t, Type*>;

#define FIELD_NAME(__name) \
    static constexpr const char* __name##Field  = #__name; \

#define FIELD_CELL_GETTER(__type, __name) \
    __type* __name() const \
    { \
        return get(__name##Field).asCell<__type>(); \
    } \

#define FIELD_CELL_SETTER(__type, __name) \
    void set_##__name(__type* __value) \
    { \
        set(__name##Field, __value); \
    } \

#define FIELD_VALUE_GETTER(__type, __name, __getter) \
    __type __name() const \
    { \
        return get(__name##Field).__getter(); \
    } \

#define FIELD_VALUE_SETTER(__type, __name) \
    void set_##__name(__type __value) \
    { \
        set(__name##Field, __value); \
    } \

#define CELL_FIELD(__type, __name) \
    FIELD_NAME(__name) \
    FIELD_CELL_GETTER(__type, __name) \
    FIELD_CELL_SETTER(__type, __name) \

#define VALUE_FIELD(__type, __name, __getter) \
    FIELD_NAME(__name) \
    FIELD_VALUE_GETTER(__type, __name, __getter) \
    FIELD_VALUE_SETTER(__type, __name) \

class Type : public Object {
public:
    enum class Class : uint8_t {
        AnyType,
        AnyValue,
        Type,
        Bottom,
        Name,
        Function,
        Array,
        Record,
        Var,
    };

    CELL_TYPE(Type);

    template<typename T, typename = std::enable_if_t<std::is_base_of<Type, T>::value>>
    bool is() const;

    template<typename T, typename = std::enable_if_t<std::is_base_of<Type, T>::value>>
    T* as();

    template<typename T, typename = std::enable_if_t<std::is_base_of<Type, T>::value>>
    const T* as() const;

    bool operator!=(const Type& other) const;
    friend std::ostream& operator<<(std::ostream&, const Type&);

    virtual Type* instantiate(VM&);
    virtual bool operator==(const Type& other) const = 0;
    virtual Type* substitute(VM&, Substitutions&) = 0;

protected:
    Type();
};

class TypeType : public Type {
public:
    CELL_CREATE(TypeType);

    Type* substitute(VM&, Substitutions&) override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;
};

class TypeBottom : public Type {
public:
    CELL_CREATE(TypeBottom);

    Type* substitute(VM&, Substitutions&) override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;
};

class TypeName : public Type {
public:
    CELL_CREATE(TypeName);

    Type* substitute(VM&, Substitutions&) override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

    CELL_FIELD(String, name);

private:
    TypeName(const std::string&);
};

class TypeVar;
class TypeFunction : public Type {
public:
    CELL_CREATE(TypeFunction);

    size_t paramCount() const;
    Type* param(uint32_t) const;
    TypeVar* implicitParam(uint32_t) const;

    Type* instantiate(VM&) override;

    Type* substitute(VM&, Substitutions&) override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

    CELL_FIELD(Array, params);
    CELL_FIELD(Array, implicitParams);
    CELL_FIELD(Array, explicitParams);
    VALUE_FIELD(uint32_t, implicitParamCount, asNumber);
    VALUE_FIELD(uint32_t, explicitParamCount, asNumber);
    CELL_FIELD(Type, returnType);

private:
    TypeFunction(const Types&, Type*);
};

class TypeArray : public Type {
public:
    CELL_CREATE(TypeArray);

    Type* substitute(VM&, Substitutions&) override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

    CELL_FIELD(Type, itemType);

private:
    TypeArray(Type*);
};

class TypeRecord : public Type {
public:
    CELL_CREATE(TypeRecord);

    Type* field(const std::string&) const;

    Type* substitute(VM&, Substitutions&) override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

    CELL_FIELD(Object, fields)

private:
    TypeRecord(const Fields&);
};

class TypeVar : public Type {
public:
    CELL_CREATE(TypeVar);

    bool isRigid() const { return m_isRigid; }

    void fresh(VM&, Substitutions&) const;

    Type* substitute(VM&, Substitutions&) override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

    VALUE_FIELD(uint32_t, uid, asNumber);
    VALUE_FIELD(bool, inferred, asBool);
    CELL_FIELD(String, name);

private:
    TypeVar(const std::string&, bool, bool);

    static uint32_t s_uid;
    bool m_isRigid;
};

template<typename T, typename>
bool Type::is() const
{
    return !!dynamic_cast<const T*>(this);
}

template<typename T, typename>
T* Type::as()
{
    ASSERT(is<T>(), "Invalid type conversion");
    return dynamic_cast<T*>(this);

}

template<typename T, typename>
const T* Type::as() const
{
    ASSERT(is<T>(), "Invalid type conversion");
    return dynamic_cast<const T*>(this);

}

std::ostream& operator<<(std::ostream&, Type::Class);
