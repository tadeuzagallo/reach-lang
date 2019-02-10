#pragma once

#include "BytecodeBlock.h"
#include "Environment.h"
#include "Instructions.h"
#include "Value.h"
#include "VM.h"
#include <vector>

class Function;

class Interpreter {
public:
    Interpreter(VM&, const BytecodeBlock&, const Environment*);
    ~Interpreter();

    VM& vm() { return m_vm; }

    Value run(std::vector<Value> = {});
    Value call(Function*, std::vector<Value>);

private:
    void dispatch();

#define DECLARE_OP(Instruction) void run##Instruction(const Instruction&);
    FOR_EACH_INSTRUCTION(DECLARE_OP)
#undef DECLARE_OP

    VM& m_vm;
    const BytecodeBlock& m_block;
    Environment* m_environment;
    bool m_stop;
    InstructionStream::Ref m_ip;
    Value* m_cfr;
    Value m_result;
};
