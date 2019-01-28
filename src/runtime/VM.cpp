#include "VM.h"

#include "Environment.h"
#include "Function.h"
#include "Type.h"
#include <iostream>

static void addFunction(VM* vm, const char* name, NativeFunction fn)
{
    Function* builtinFunction = Function::create(*vm, fn);
    vm->globalEnvironment.set(name, Value { builtinFunction });
}

static Value functionPrint(VM& vm, std::vector<Value> args)
{
    bool isFirst = true;
    for (auto arg : args) {
        if (!isFirst)
            std::cout << ", ";
        std::cout << arg;
        isFirst = false;
    }
    std::cout << std::endl;
    return Value::unit();
}

VM::VM()
    : globalEnvironment(nullptr)
    , heap(this)
    , typeChecker(nullptr)
{
    // so we don't crash when calling stack.back()
    stack.push_back(Value::crash());

    addFunction(this, "print", functionPrint);
}

void VM::addType(const std::string& name, const Type& type)
{
    globalEnvironment.set(name, Value { &type });
}

