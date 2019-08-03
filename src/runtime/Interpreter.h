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

    using Values = std::vector<Value>;
    using Callback = std::function<void(const Interpreter&)>;

public:
    static Value check(VM& vm, BytecodeBlock&, Environment*);
    static Value run(VM& vm, BytecodeBlock&, Environment* = nullptr, const Values& = {}, const Callback& = {});

    void visit(const Visitor&) const;

    Environment* environment() const { return m_environment; }

private:
    enum class Mode {
        Check = 0b01,
        Run = 0b10,
        CheckAndRun = 0b11,
    };

    Interpreter(VM&, BytecodeBlock&, InstructionStream::Offset, Environment*);
    ~Interpreter();

    VM& vm() { return m_vm; }

    Value run(const Values& = {}, const Callback& = {});

    template<typename Functor>
    Value preserveStack(const Functor&);

    void dispatch();

#define DECLARE_OP(Instruction) void run##Instruction(const Instruction&);
    FOR_EACH_INSTRUCTION(DECLARE_OP)
#undef DECLARE_OP

    struct Stack {
        Value& operator[](const Register&) const;

        Value* m_stackAddress;
    };

    VM& m_vm;
    BytecodeBlock& m_block;
    const BytecodeBlock* m_lastBlock;
    Environment* m_environment;
    Interpreter* m_lastInterpreter;
    Mode m_mode { Mode::Run };
    bool m_stop;
    InstructionStream::Ref m_ip;
    Stack m_cfr;
    Value m_result;
    Callback m_callback;
};
