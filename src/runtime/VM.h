#pragma once

#include "Environment.h"
#include "Heap.h"
#include "Value.h"
#include <vector>

class BytecodeBlock;
class Value;

class VM {
public:
    VM();


public:
    BytecodeBlock* globalBlock;
    Environment globalEnvironment;
    Heap heap;
    std::vector<Value> stack;
};
