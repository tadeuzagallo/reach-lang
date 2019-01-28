#pragma once

#include "Cell.h"
#include "VM.h"
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>

class Binding;
class Type;
class TypeChecker;

using Types = std::vector<Binding>;
using Fields = std::unordered_map<std::string, Binding>;
using Substitutions = std::unordered_map<uint32_t, const Type&>;

class Type : public Cell {
    friend class TypeChecker;

public:
    CELL_TYPE(Type);

    template<typename T, typename = std::enable_if_t<std::is_base_of<Type, T>::value>>
    bool is() const;
    template<typename T, typename = std::enable_if_t<std::is_base_of<Type, T>::value>>
    const T& as() const;

    bool operator!=(const Type& other) const;
    friend std::ostream& operator<<(std::ostream&, const Type&);

    virtual const Type& instantiate(TypeChecker&) const;

    virtual bool operator==(const Type& other) const = 0;
    virtual const Type& substitute(TypeChecker&, Substitutions&) const = 0;
};

class TypeType : public Type {
public:
    CELL_CREATE(TypeType);

    void visit(std::function<void(Value)>) const override;
    const Type& substitute(TypeChecker&, Substitutions&) const override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;
};

class TypeBottom : public Type {
public:
    CELL_CREATE(TypeBottom);

    void visit(std::function<void(Value)>) const override;
    const Type& substitute(TypeChecker&, Substitutions&) const override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;
};

class TypeName : public Type {
    friend class TypeChecker;

public:
    CELL_CREATE(TypeName);

    void visit(std::function<void(Value)>) const override;
    const Type& substitute(TypeChecker&, Substitutions&) const override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

private:
    TypeName(const std::string&);

    std::string m_name;
};

class TypeFunction : public Type {
    friend class TypeChecker;

public:
    CELL_CREATE(TypeFunction);

    size_t paramCount() const;
    size_t explicitParamCount() const;
    const Binding& param(uint32_t) const;
    const Type& returnType() const;

    const Type& instantiate(TypeChecker&) const override;

    void visit(std::function<void(Value)>) const override;
    const Type& substitute(TypeChecker&, Substitutions&) const override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

private:
    TypeFunction(const Types&, const Type&);

    Types m_params;
    const Type& m_returnType;
};

class TypeArray : public Type {
    friend class TypeChecker;

public:
    CELL_CREATE(TypeArray);

    const Type& itemType() const;

    void visit(std::function<void(Value)>) const override;
    const Type& substitute(TypeChecker&, Substitutions&) const override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

private:
    TypeArray(const Type&);

    const Type& m_itemType;
};

class TypeRecord : public Type {
    friend class TypeChecker;

public:
    CELL_CREATE(TypeRecord);

    std::optional<std::reference_wrapper<const Binding>> field(const std::string&) const;

    void visit(std::function<void(Value)>) const override;
    const Type& substitute(TypeChecker&, Substitutions&) const override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

private:
    TypeRecord(const Fields&);

    Fields m_fields;
};

class TypeVar : public Type {
    friend class TypeChecker;

public:
    CELL_CREATE(TypeVar);

    uint32_t uid() const;
    bool inferred() const;

    void fresh(TypeChecker&, Substitutions&) const;

    void visit(std::function<void(Value)>) const override;
    const Type& substitute(TypeChecker&, Substitutions&) const override;
    bool operator==(const Type&) const override;
    void dump(std::ostream&) const override;

private:
    TypeVar(const std::string&, bool);

    static uint32_t s_uid;

    uint32_t m_uid;
    bool m_inferred;
    std::string m_name;
};

template<typename T, typename>
bool Type::is() const
{
    return !!dynamic_cast<const T*>(this);
}


template<typename T, typename>
const T& Type::as() const
{
    ASSERT(is<T>(), "Invalid type conversion");
    return dynamic_cast<const T&>(*this);

}
