#pragma once

#include "Heap.h"
#include "Value.h"
#include <vector>

class BytecodeBlock;
class Environment;
class Type;
class TypeChecker;
class Value;

class VM {
public:
    VM();

    void addType(const std::string&, const Type&);

    Environment* globalEnvironment;
    BytecodeBlock* globalBlock;
    TypeChecker* typeChecker;
    Heap heap;
    std::vector<Value> stack;
};
