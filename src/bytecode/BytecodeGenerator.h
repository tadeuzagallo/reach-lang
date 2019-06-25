#pragma once

#include "BytecodeBlock.h"
#include "Label.h"
#include "Register.h"
#include "Value.h"
#include "VM.h"
#include "expressions.h"
#include <string>
#include <memory>
#include <map>

class BytecodeGenerator {
public:
    BytecodeGenerator(VM&, std::string = "<global>");

    VM& vm() { return m_vm; }
    BytecodeBlock& block() { return *m_block; }

    std::unique_ptr<BytecodeBlock> finalize(Register);
    Register newLocal();
    Label label();
    void branch(Register, const std::function<void()>&, const std::function<void()>&);

    void emitLocation(const SourceLocation&);
    void emitPrologue(const std::function<void()>&);

    void loadConstant(Register, Value);
    void loadConstantIndex(Register, uint32_t);
    uint32_t storeConstant(Register);
    void getLocal(Register, const Identifier&);
    void setLocal(const Identifier&, Register);
    void getLocal(Register, const std::string&);
    void setLocal(const std::string&, Register);
    void call(Register, Register, const std::vector<Register>&);
    void newArray(Register, unsigned);
    void setArrayIndex(Register, unsigned, Register);
    void getArrayIndex(Register, Register, Register);
    void getArrayLength(Register, Register);
    void newTuple(Register, unsigned);
    void setTupleIndex(Register, unsigned, Register);
    void getTupleIndex(Register, Register, Register);
    uint32_t newFunction(Register, std::unique_ptr<BytecodeBlock>);
    void newFunction(Register, uint32_t);
    void move(Register dst, Register src);
    void newObject(Register, uint32_t);
    void setField(Register, const std::string&, Register);
    void getField(Register, Register, const std::string&);
    void jump(Label&);
    void jumpIfFalse(Register, Label&);
    void isEqual(Register, Register, Register);

    // Type checking operations
    void pushScope();
    void popScope();
    void pushUnificationScope();
    void popUnificationScope();
    void unify(Register, Register);
    void resolveType(Register, Register);
    void checkType(Register, Register, Type::Class);
    void checkTypeOf(Register, Register, Type::Class);
    void typeError(const SourceLocation&, const char*);
    void inferImplicitParameters(Register, const std::vector<Register>&);
    void endTypeChecking(Register);

    // Types
    void newVarType(Register, const std::string&, bool);
    void newNameType(Register, const std::string&);
    void newArrayType(Register, Register);
    void newTupleType(Register, unsigned);
    void newRecordType(Register, const std::vector<std::pair<std::string, Register>>&);
    void newFunctionType(Register, const std::vector<Register>&, Register, uint32_t);

    void newValue(Register, Register);
    void getTypeForValue(Register, Register);

    void emit(Instruction::ID);
    void emit(Register);
    void emit(Label&);
    void emit(uint32_t);
    void emit(Type::Class);

private:
    template<typename Instruction, typename... Args>
    void emit(Args&&... args)
    {
        Instruction::emit(this, std::forward<Args>(args)...);
    }

    uint32_t uniqueIdentifier(const std::string&);

    VM& m_vm;
    std::unique_ptr<BytecodeBlock> m_block;
    std::map<std::string, uint32_t> m_uniqueIdentifierMapping;
};
