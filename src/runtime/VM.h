#pragma once

#include "Heap.h"
#include "InstructionStream.h"
#include "LocationInfo.h"
#include "Value.h"
#include <vector>

class BytecodeBlock;
class Environment;
class Interpreter;
class Scope;
class Type;
class TypeChecker;
class UnificationScope;
class Value;

class VM {
public:
    VM();

    void typeError(InstructionStream::Offset, const std::string&);
    void runtimeError(InstructionStream::Offset, const std::string&);
    bool hasTypeErrors() const;
    bool reportTypeErrors();

    Environment* globalEnvironment;
    Interpreter* currentInterpreter { nullptr };
    BytecodeBlock* globalBlock;
    const BytecodeBlock* currentBlock;
    TypeChecker* typeChecker { nullptr };

    Heap heap;
    std::vector<Value> stack;

    // TypeChecking business
    Scope* typingScope { nullptr };
    UnificationScope* unificationScope { nullptr };

    Type* stringType;
    Type* typeType;
    Type* topType;
    Type* bottomType;
    Type* unitType;
    Type* boolType;
    Type* numberType;

private:
    struct TypeError {
        LocationInfoWithFile locationInfo;
        std::string message;
    };

    std::vector<TypeError> m_typeErrors;
};
