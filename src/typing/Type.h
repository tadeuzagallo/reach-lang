#pragma once

#include <string>
#include <vector>
#include <iostream>

class Type;
class TypeFunction;
class TypeName;

using Types = std::vector<std::reference_wrapper<const Type>>;

class Type {
public:
    static Type* named(const std::string&);
    static Type* function(Types, const Type&);

    bool operator==(const Type& other) const;
    bool operator!=(const Type& other) const;

    bool isName() const;
    bool isFunction() const;

    const TypeName& asName() const;
    const TypeFunction& asFunction() const;

    void dump(std::ostream&) const;
    friend std::ostream& operator<<(std::ostream&, const Type&);

protected:
    enum class Tag {
        Name,
        Function,
    };

    Type(Tag);

    Tag m_tag;
};

class TypeName : public Type {
public:
    TypeName(const std::string&);

    bool operator==(const TypeName&) const;
    void dump(std::ostream&) const;

private:
    std::string m_name;
};

class TypeFunction : public Type {
public:
    TypeFunction(Types, const Type&);

    size_t paramCount() const;
    const Type& param(uint32_t) const;
    const Type& returnType() const;

    bool operator==(const TypeFunction&) const;
    void dump(std::ostream&) const;

private:
    Types m_params;
    const Type& m_returnType;
};
