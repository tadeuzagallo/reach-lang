#pragma once

#include "Array.h"
#include "Hole.h"
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

class Type : public Object {
    friend class JIT;

public:
    enum class Class : uint32_t {
        AnyValue = 0,
        AnyType = 1,
        SpecificType = 2,
        Type = 2,
        Top,
        Bottom,
        Name,
        Function,
        Array,
        Record,
        Var,
        Tuple,
        Union,
        Hole,
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

class TypeTop : public Type {
public:
    CELL_CREATE(TypeTop);

    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

private:
    TypeTop();
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

    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

    VALUE_FIELD(Value, itemType);

private:
    TypeArray(Value);
};

class TypeTuple : public Type {
public:
    CELL_CREATE(TypeTuple);

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

class TypeUnion : public Type {
public:
    CELL_CREATE(TypeUnion);

    Type* collapse(VM&);

    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

    VALUE_FIELD(Value, lhs);
    VALUE_FIELD(Value, rhs);

private:
    TypeUnion(Value, Value);
};

class TypeHole : public Type {
public:
    CELL_CREATE(TypeHole);

    Type* substitute(VM&, Substitutions&) override;
    void dump(std::ostream&) const override;

    CELL_FIELD(Hole, hole);

private:
    TypeHole(Hole*);
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
