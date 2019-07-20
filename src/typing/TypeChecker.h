#pragma once

#include "Register.h"
#include "SourceLocation.h"
#include "Type.h"
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class Binding;
class BytecodeGenerator;
class FunctionDeclaration;
class Program;
class VM;

#define FOR_EACH_BASE_TYPE(macro) \
    macro(type) \
    macro(top) \
    macro(bottom) \
    macro(unit) \
    macro(bool) \
    macro(number) \
    macro(string) \

class TypeChecker {
public:
    class Scope;
    class UnificationScope;
    friend Scope;
    friend UnificationScope;

    enum class Mode { Function, Program };

    TypeChecker(BytecodeGenerator&, bool doNotEmitScopeInstructions = false);
    ~TypeChecker();

    VM& vm() const;
    TypeChecker* previousTypeChecker() const;
    BytecodeGenerator& generator() const;
    TypeChecker::Scope& currentScope() const;
    void endTypeChecking(Mode, Register);

    void check(Program&);

#define DECLARE_LAZY_TYPE_GETTER(type) \
    Register type##Type();
FOR_EACH_BASE_TYPE(DECLARE_LAZY_TYPE_GETTER)
#undef DECLARE_LAZY_TYPE_GETTER

#define DECLARE_TYPE_VALUE_GETTER(type) \
    void type##Value(Register);
FOR_EACH_BASE_TYPE(DECLARE_TYPE_VALUE_GETTER)
#undef DECLARE_TYPE_VALUE_GETTER

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

    template<typename T>
    void inferAsType(T&, Register);

    class Scope {
    public:
        Scope(TypeChecker&);
        Scope(TypeChecker&, bool shouldGenerateBytecode);
        ~Scope();

        void addFunction(const FunctionDeclaration&);
        std::optional<std::vector<bool>> getFunction(const std::string&);

        void bindParameter(const std::string&);
        void lookup(Register, const std::string&);

    private:
        bool m_shouldGenerateBytecode;
        Scope* m_previousScope;
        TypeChecker& m_typeChecker;
        std::unordered_map<std::string, std::vector<bool>> m_functions;
        std::unordered_set<std::string> m_boundParameters;
    };

    class UnificationScope {
    public:
        UnificationScope(TypeChecker&);
        UnificationScope(TypeChecker&, bool shouldGenerateBytecode);
        ~UnificationScope();

        void resolve(Register, Register);
        void finalize();

    private:
        bool m_shouldGenerateBytecode;
        TypeChecker* m_typeChecker;
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

#define DECLARE_TYPE_FIELD(type) \
    std::optional<Register> m_##type##Type;
FOR_EACH_BASE_TYPE(DECLARE_TYPE_FIELD)
#undef DECLARE_TYPE_VALUE_GETTER

    bool m_doNotEmitScopeInstructions;
    BytecodeGenerator& m_generator;
    Scope* m_currentScope { nullptr };
    Scope m_topScope;
    UnificationScope m_topUnificationScope;
    TypeChecker* m_previousTypeChecker;
};
