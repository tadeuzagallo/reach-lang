#pragma once

#include <string>
#include <vector>
#include <iostream>

class Type;
class TypeArray;
class TypeFunction;
class TypeName;

using Types = std::vector<std::reference_wrapper<const Type>>;

class Type {
    friend class TypeChecker;

public:
    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const;

    bool isName() const;
    bool isFunction() const;
    bool isArray() const;

    const TypeName& asName() const;
    const TypeFunction& asFunction() const;
    const TypeArray& asArray() const;

    void dump(std::ostream&) const;
    friend std::ostream& operator<<(std::ostream&, const Type&);

protected:
    enum class Tag : uint8_t {
        Name,
        Function,
        Array,
    };

    Type(Tag, uint32_t);

private:
    Tag m_tag;
    uint32_t m_offset;
};

class TypeName : public Type {
    friend class TypeChecker;

public:
    bool operator==(const TypeName&) const;
    void dump(std::ostream&) const;

private:
    TypeName(uint32_t, const std::string&);

    std::string m_name;
};

class TypeFunction : public Type {
    friend class TypeChecker;;

public:
    size_t paramCount() const;
    const Type& param(uint32_t) const;
    const Type& returnType() const;

    bool operator==(const TypeFunction&) const;
    void dump(std::ostream&) const;

private:
    TypeFunction(uint32_t, Types, const Type&);

    Types m_params;
    const Type& m_returnType;
};

class TypeArray : public Type {
    friend class TypeChecker;

public:
    const Type& itemType() const;

    bool operator==(const TypeArray&) const;
    void dump(std::ostream&) const;

private:
    TypeArray(uint32_t, const Type&);

    const Type& m_itemType;
};
