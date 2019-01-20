#pragma once

#include "SourceLocation.h"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Program;
class Type;

class TypeChecker {
public:
    TypeChecker();

    const Type* check(const std::unique_ptr<Program>&);

    const Type& unitType();
    const Type& booleanType();
    const Type& numericType();
    const Type& stringType();

    const Type& lookup(const SourceLocation&, const std::string&);
    void insert(const std::string&, const Type*);

    void checkEquals(const SourceLocation&, const Type&, const Type&);
    void typeError(const SourceLocation&, const std::string&);
    void reportErrors(std::ostream&) const;

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
    std::unordered_map<std::string, const Type*> m_environment;
};
