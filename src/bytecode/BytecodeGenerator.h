#pragma once

#include "BytecodeBlock.h"
#include "Label.h"
#include "Register.h"
#include "Value.h"
#include "VM.h"
#include "expressions.h"
#include <memory>
#include <map>

class BytecodeGenerator {
public:
    BytecodeGenerator(VM&, std::string);

    VM& vm() { return m_vm; }

    void emitReturn();

    std::unique_ptr<BytecodeBlock> finalize(Register);
    Register newLocal();
    Label label();

    void loadConstant(Register, Value);
    void getLocal(Register, const Identifier&);
    void setLocal(const Identifier&, Register);
    void call(Register, Register, std::vector<Register>&);
    void newArray(Register, unsigned);
    void setArrayIndex(Register, unsigned, Register);
    void getArrayIndex(Register, Register, Register);
    void newFunction(Register, std::unique_ptr<BytecodeBlock>);
    void move(Register dst, Register src);
    void newObject(Register, uint32_t);
    void setField(Register, const Identifier&, Register);
    void getField(Register, Register, const Identifier&);
    void jump(Label&);
    void jumpIfFalse(Register, Label&);

    void emit(Instruction::ID);
    void emit(Register);
    void emit(Label&);
    void emit(uint32_t);

private:
    template<typename Instruction, typename... Args>
    void emit(Args&&... args)
    {
        Instruction::emit(this, std::forward<Args>(args)...);
    }

    uint32_t uniqueIdentifier(const Identifier&);

    VM& m_vm;
    std::unique_ptr<BytecodeBlock> m_block;
    std::map<Identifier, uint32_t> m_uniqueIdentifierMapping;
};
