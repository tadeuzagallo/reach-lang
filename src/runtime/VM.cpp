#include "VM.h"

#include "Environment.h"
#include "Function.h"
#include "Type.h"
#include <iostream>
#include <sstream>

static void addFunction(VM* vm, const char* name, NativeFunction fn, Type* type)
{
    Function* builtinFunction = Function::create(*vm, fn, type);
    vm->globalEnvironment->set(name, Value { builtinFunction });
}

static Value functionPrint(VM&, std::vector<Value> args)
{
    ASSERT(args.size() == 1, "print expects a single argument");
    ASSERT(args[0].isCell<String>(), "print expects a string as its first argument");
    std::cout << args[0].asCell<String>()->str();
    return Value::unit();
}

static Value functionPrintln(VM&, std::vector<Value> args)
{
    ASSERT(args.size() == 1, "println expects a single argument");
    ASSERT(args[0].isCell<String>(), "println expects a string as its first argument");
    std::cout << args[0].asCell<String>()->str() << std::endl;;
    return Value::unit();
}

static Value functionStringify(VM& vm, std::vector<Value> args)
{
    ASSERT(args.size() == 1, "stringify expects a single argument");
    std::stringstream str;
    args[0].dump(str);
    return String::create(vm, str.str());
}

VM::VM()
    : typeChecker(nullptr)
    , heap(this)
    , stringType(nullptr)
    , typeType(TypeType::create(*this))
    , topType(TypeTop::create(*this))
    , bottomType(TypeBottom::create(*this))
    , unitType(TypeName::create(*this, "Void"))
    , boolType(TypeName::create(*this, "Bool"))
    , numberType(TypeName::create(*this, "Number"))
{
    // Break the cycle VM -> Type -> String -> VM
    stringType = TypeName::create(*this, "String");
    globalEnvironment = Environment::create(*this, nullptr);

    // so we don't crash when calling stack.back()
    stack.push_back(Value::crash());

    Value stringValue = stringType;
    auto* printType = TypeFunction::create(*this, 1, &stringValue, unitType, 0);
    addFunction(this, "print", functionPrint, printType);
    addFunction(this, "println", functionPrintln, printType);

    Value topValue = topType;
    auto* stringifyType = TypeFunction::create(*this, 1, &topValue, stringType, 0);
    addFunction(this, "stringify", functionStringify, stringifyType);

    globalEnvironment->set("Void", unitType);
    globalEnvironment->set("Bool", boolType);
    globalEnvironment->set("Number", numberType);
    globalEnvironment->set("String", stringType);
}

void VM::typeError(InstructionStream::Offset bytecodeOffset, const std::string& message)
{
    m_typeErrors.emplace_back(TypeError { currentBlock->locationInfo(bytecodeOffset), message });
}

bool VM::hasTypeErrors() const
{
    return !m_typeErrors.empty();
}

bool VM::reportTypeErrors()
{
    if (m_typeErrors.empty())
        return false;

    for (const auto& typeError : m_typeErrors)
        std::cerr << typeError.locationInfo << ": " << typeError.message << std::endl;
    m_typeErrors.clear();
    return true;
}
