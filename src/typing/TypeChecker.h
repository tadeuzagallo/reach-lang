#pragma once

#include "SourceLocation.h"
#include "Type.h"
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <vector>

class Program;

class TypeChecker {
    friend class Scope;

public:
    TypeChecker();

    const Type* check(const std::unique_ptr<Program>&);

    const Type& unitType();
    const Type& booleanType();
    const Type& numericType();
    const Type& stringType();

    const TypeName& newNameType(const std::string&);
    const TypeFunction& newFunctionType(Types, const Type&);
    const TypeArray& newArrayType(const Type&);
    const TypeRecord& newRecordType(const Fields&);

    const Type& lookup(const SourceLocation&, const std::string&);
    void insert(const std::string&, const Type&);

    void checkEquals(const SourceLocation&, const Type&, const Type&);
    void typeError(const SourceLocation&, const std::string&);
    void reportErrors(std::ostream&) const;

    class Scope {
    public:
        Scope(TypeChecker&);
        ~Scope();

    private:
        TypeChecker& m_typeChecker;
        size_t m_environmentSize;
        size_t m_typesSize;
    };

private:
    class Error {
    public:
        Error(const SourceLocation&, const std::string&);

        const SourceLocation& location() const;
        const std::string& message() const;

    private:
        SourceLocation m_location;
        std::string m_message;
    };

    std::vector<Error> m_errors;
    std::vector<std::pair<std::string, uint32_t>> m_environment;
    std::vector<Type*> m_types;
};
