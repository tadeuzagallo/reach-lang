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

class BytecodeBlock;
class Type;

using Types = std::vector<Value>;
using Fields = std::unordered_map<std::string, Value>;
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

#define FIELD_VALUE_GETTER(__type, __name, ...) \
    __type __name() const \
    { \
        return get(__name##Field) __VA_ARGS__; \
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

#define VALUE_FIELD(__type, __name, ...) \
    FIELD_NAME(__name) \
    FIELD_VALUE_GETTER(__type, __name, __VA_ARGS__) \
    FIELD_VALUE_SETTER(__type, __name) \

class Type : public Object {
    friend class JIT;

public:
    enum class Class : uint8_t {
        AnyValue = 0,
        AnyType = 1,
        SpecificType = 2,
        Type = 2,
        Bottom,
        Name,
        Function,
        Array,
        Record,
        Var,
        Tuple,
    };

    CELL_TYPE(Type);

    Class typeClass() const { return m_class; }

    template<typename T, typename = std::enable_if_t<std::is_base_of<Type, T>::value>>
    bool is() const;

    template<typename T, typename = std::enable_if_t<std::is_base_of<Type, T>::value>>
    T* as();

    template<typename T, typename = std::enable_if_t<std::is_base_of<Type, T>::value>>
    const T* as() const;

    friend std::ostream& operator<<(std::ostream&, const Type&);

    bool operator<=(const Type& other) const;

    virtual Type* instantiate(VM&);
    virtual Type* substitute(VM&, Substitutions&) = 0;

protected:
    Type(Class);

    Class m_class;
};

class TypeType : public Type {
public:
    CELL_CREATE(TypeType);

    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

private:
    TypeType();
};

class TypeBottom : public Type {
public:
    CELL_CREATE(TypeBottom);

    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

private:
    TypeBottom();
};

class TypeName : public Type {
public:
    CELL_CREATE(TypeName);

    bool operator==(const TypeName&) const;
    Type* substitute(VM&, Substitutions&) override;
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

    bool operator<=(const TypeFunction&) const;
    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

    CELL_FIELD(Array, params);
    CELL_FIELD(Array, implicitParams);
    CELL_FIELD(Array, explicitParams);
    VALUE_FIELD(uint32_t, implicitParamCount, .asNumber());
    VALUE_FIELD(uint32_t, explicitParamCount, .asNumber());
    VALUE_FIELD(Value, returnType);

private:
    TypeFunction(uint32_t, const Value*, Value, uint32_t);

    uint32_t m_inferredParameters;
};

class TypeArray : public Type {
public:
    CELL_CREATE(TypeArray);

    bool operator<=(const TypeArray&) const;
    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

    VALUE_FIELD(Value, itemType);

private:
    TypeArray(Value);
};

class TypeTuple : public Type {
public:
    CELL_CREATE(TypeTuple);

    bool operator<=(const TypeTuple&) const;
    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

    CELL_FIELD(Array, itemsTypes);

private:
    TypeTuple(uint32_t);
};

class TypeRecord : public Type {
public:
    CELL_CREATE(TypeRecord);

    Type* field(const std::string&) const;

    bool operator<=(const TypeRecord&) const;
    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

    CELL_FIELD(Object, fields)

private:
    TypeRecord(const Fields&);
    TypeRecord(const BytecodeBlock&, uint32_t, const Value*, const Value*);
};

class TypeVar : public Type {
public:
    CELL_CREATE(TypeVar);

    bool isRigid() const { return m_isRigid; }

    void fresh(VM&, Substitutions&) const;

    bool operator==(const TypeVar&) const;
    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

    VALUE_FIELD(uint32_t, uid, .asNumber());
    VALUE_FIELD(bool, inferred, .asBool());
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

extern "C" {
// JIT helpers
TypeVar* createTypeVar(VM&, const std::string&, bool, bool);
TypeName* createTypeName(VM&, const std::string&);
TypeArray* createTypeArray(VM&, Value);
TypeRecord* createTypeRecord(VM&, const BytecodeBlock&, uint32_t, const Value*, const Value*);
TypeFunction* createTypeFunction(VM&, uint32_t, const Value*, Value, uint32_t);
};
