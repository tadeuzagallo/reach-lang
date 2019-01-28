#pragma once

#include "Environment.h"
#include "Heap.h"
#include "Value.h"
#include <vector>

class BytecodeBlock;
class Type;
class TypeChecker;
class Value;

class VM {
public:
    VM();

    void addType(const std::string&, const Type&);

public:
    BytecodeBlock* globalBlock;
    Environment globalEnvironment;
    TypeChecker* typeChecker;
    Heap heap;
    std::vector<Value> stack;
};
