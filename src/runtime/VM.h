#pragma once

#include "Heap.h"
#include "Value.h"
#include <vector>

class BytecodeBlock;
class Environment;
class Scope;
class Type;
class TypeChecker;
class UnificationScope;
class Value;

class VM {
public:
    VM();

    Environment* globalEnvironment;
    BytecodeBlock* globalBlock;
    TypeChecker* typeChecker;
    Heap heap;
    std::vector<Value> stack;

    // TypeChecking business
    Scope* typingScope { nullptr }; // TODO: s/scope/typingScope/
    UnificationScope* unificationScope { nullptr };

    Type* typeType;
    Type* bottomType;
    Type* unitType;
    Type* boolType;
    Type* numberType;
    Type* stringType;
};
