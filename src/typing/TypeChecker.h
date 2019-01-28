#pragma once

#include "SourceLocation.h"
#include "Type.h"
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Binding;
class Program;
class VM;

class TypeChecker {
    friend class Scope;
    friend class UnificationScope;

public:
    TypeChecker(VM&);
    VM& vm() const;

    const Type* check(const std::unique_ptr<Program>&);
    void visit(const std::function<void(Value)>&) const;

    const Binding& typeType();

    const Binding& unitValue();
    const Binding& booleanValue();
    const Binding& numericValue();
    const Binding& stringValue();
    const Binding& bottomValue();

    const Binding& newValue(const Type&);
    const Binding& newValue(const Binding&);
    const Binding& newFunctionValue(const Types&, const Type&);
    const Binding& newArrayValue(const Type&);
    const Binding& newRecordValue(const Fields&);

    const Binding& newNameType(const std::string&);
    const Binding& newVarType(const std::string&);

    const Binding& lookup(const SourceLocation&, const std::string&);

    void bindValue(const Type&);
    void bindType(const Type&);

    void insert(const std::string&, const Binding&);

    void unify(const SourceLocation&, const Binding&, const Binding&);
    void typeError(const SourceLocation&, const std::string&);
    void reportErrors(std::ostream&) const;

    class Scope {
    public:
        Scope(TypeChecker&);
        ~Scope();

    private:
        TypeChecker& m_typeChecker;
        size_t m_environmentSize;
        size_t m_bindingsSize;
    };

    class UnificationScope {
    public:
        UnificationScope(TypeChecker&);
        ~UnificationScope();

        void unify(const SourceLocation&, const Binding&, const Binding&);
        const Type& result(const Type&);

    private:
        struct Constraint {
            SourceLocation location;
            const Binding& lhs;
            const Binding& rhs;
        };

        void solveConstraints();
        void unifies(const SourceLocation&, const Type&, const Type&);
        void bind(const TypeVar&, const Type&);

        bool m_finalized { false };
        TypeChecker& m_typeChecker;
        UnificationScope* m_parentScope;
        std::deque<Constraint> m_constraints;
        Substitutions m_substitutions;
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

    VM& m_vm;
    Scope m_topScope;
    UnificationScope m_topUnificationScope;
    UnificationScope* m_unificationScope;
    std::vector<Error> m_errors;
    std::vector<std::pair<std::string, const Binding*>> m_environment;
    std::vector<Binding*> m_bindings;
};
