#include "VM.h"

#include "Environment.h"
#include "Function.h"
#include "Type.h"
#include <iostream>

static void addFunction(VM* vm, const char* name, NativeFunction fn, Type* type)
{
    Function* builtinFunction = Function::create(*vm, fn, type);
    vm->globalEnvironment->set(name, Value { builtinFunction });
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

static Value functionInspect(VM& vm, std::vector<Value> args)
{
    ASSERT(args.size() % 2 == 0, "OOPS");
    bool isFirst = true;
    for (uint32_t i = 0; i < args.size(); i += 2) {
        if (!isFirst)
            std::cout << ", ";
        std::cout << args[i] << " : " << *args[i + 1].type(vm);
        isFirst = false;
    }
    std::cout << std::endl;
    return Value::unit();
}

VM::VM()
    : heap(this)
    , typeChecker(nullptr)
    , typeType(TypeType::create(*this))
    , bottomType(TypeBottom::create(*this))
    , unitType(TypeName::create(*this, "Void"))
    , boolType(TypeName::create(*this, "Bool"))
    , numberType(TypeName::create(*this, "Number"))
    , stringType(TypeName::create(*this, "String"))
{
    globalEnvironment = Environment::create(*this, nullptr);

    // so we don't crash when calling stack.back()
    stack.push_back(Value::crash());

    addFunction(this, "print", functionPrint, bottomType);
    addFunction(this, "inspect", functionInspect, bottomType);
}
