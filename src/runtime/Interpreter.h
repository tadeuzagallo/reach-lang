#pragma once

#include "BytecodeBlock.h"
#include "Environment.h"
#include "Instructions.h"
#include "Value.h"
#include "VM.h"
#include <vector>

class Function;

class Interpreter {
    friend class Scope;

public:
    Interpreter(VM&, const BytecodeBlock&, Environment*);
    ~Interpreter();

    VM& vm() { return m_vm; }

    Value run(std::vector<Value> = {}, const std::function<void()>& = {});
    Value call(Function*, std::vector<Value>);
    Value reg(Register) const;

private:
    void dispatch();

#define DECLARE_OP(Instruction) void run##Instruction(const Instruction&);
    FOR_EACH_INSTRUCTION(DECLARE_OP)
#undef DECLARE_OP

    struct Stack {
        Value& operator[](const Register&) const;

        Value* m_stackAddress;
    };

    VM& m_vm;
    const BytecodeBlock& m_block;
    Environment* m_environment;
    bool m_stop;
    InstructionStream::Ref m_ip;
    Stack m_cfr;
    Value m_result;
    std::function<void()> m_callback;
};
