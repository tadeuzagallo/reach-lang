#pragma once

#include "Register.h"
#include "SourceLocation.h"
#include "Type.h"
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Binding;
class BytecodeGenerator;
class Program;
class VM;

class TypeChecker {
    friend class Scope;
    friend class UnificationScope;

public:
    TypeChecker(BytecodeGenerator&);
    VM& vm() const;
    BytecodeGenerator& generator();

    Register check(const std::unique_ptr<Program>&);
    void visit(const std::function<void(Value)>&) const;

    Register typeType();
    Register unitType();
    Register numberType();

    void typeType(Register);
    void unitType(Register);

    void unitValue(Register);
    void booleanValue(Register);
    void numberValue(Register);
    void stringValue(Register);
    void bottomValue(Register);

    void newType(Register result, Register type);
    void newValue(Register result, Register type);

    void newFunctionValue(Register result, const std::vector<Register>&, Register);
    void newArrayValue(Register result, Register itemType);
    void newRecordValue(Register result, const std::vector<std::pair<std::string, Register>>&);

    void newNameType(Register, const std::string&);
    void newVarType(Register, const std::string&, bool);
    void newArrayType(Register, Register);
    void newRecordType(Register, const std::vector<std::pair<std::string, Register>>&);
    void newFunctionType(Register result, const std::vector<Register>&, Register);

    void lookup(Register, const SourceLocation&, const std::string&);

    void insert(const std::string&, Register);

    void unify(const SourceLocation&, Register, Register);
    //void typeError(const SourceLocation&, const std::string&);
    //void reportErrors(std::ostream&) const;

    template<typename T>
    void inferAsType(const T&, Register);

    class Scope {
    public:
        Scope(TypeChecker&);
        ~Scope();

    private:
        TypeChecker& m_typeChecker;
    };

    class UnificationScope {
    public:
        UnificationScope(TypeChecker&);
        ~UnificationScope();

        void resolve(Register, Register);

    private:
        TypeChecker& m_typeChecker;
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

    Register m_typeType;
    Register m_bottomType;
    Register m_unitType;
    Register m_boolType;
    Register m_numberType;
    Register m_stringType;

    BytecodeGenerator& m_generator;
    //Scope m_topScope;
    UnificationScope m_topUnificationScope;

    //std::vector<Error> m_errors;
    //std::vector<std::pair<std::string, const Binding*>> m_environment;
    //std::vector<Binding*> m_bindings;
};
