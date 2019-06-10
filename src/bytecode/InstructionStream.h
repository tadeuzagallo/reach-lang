#pragma once

#include "Instruction.h"
#include <iostream>
#include <vector>

#define OFFSETOF(__obj, __field) \
    (((uintptr_t)&(((__obj*)0xbbadbeef)->__field)) - 0xbbadbeefll)

class InstructionStream {
    friend class BytecodeGenerator;
    friend class WritableRef;

public:
    using Offset = size_t;

    InstructionStream();

    class Ref {
        friend class InstructionStream;

    public:
        const Instruction* get() const;
        const Instruction* operator->() const;
        const Ref& operator*() const;
        bool operator!=(const Ref& other) const;
        Ref operator++();
        Ref& operator+=(int32_t);
        Offset offset() const;
        void dump(std::ostream&) const;

    private:
        Ref(const InstructionStream&, Offset);

        const InstructionStream& m_instructions;
        Offset m_offset;
    };

    class WritableRef {
        friend class InstructionStream;

    public:
        void write(uint32_t value);
        Offset offset() const;
        WritableRef& operator+=(int32_t);

    private:
        WritableRef(InstructionStream&, Offset, uint32_t);

        InstructionStream& m_instructions;
        Offset m_instructionOffset;
        uint32_t m_targetOffset;
    };

    Ref at(Offset) const;
    Ref begin() const;
    Ref end() const;

    size_t size() const { return m_instructions.size(); }

    template<typename JumpType, typename Label>
    void recordJump(uint32_t prologueSize, Label& label)
    {
        Offset offset = m_instructions.size();
        label.addReference(offset, prologueSize, WritableRef { *this, offset, OFFSETOF(JumpType, target) >> 2 });
    }

    void dump(std::ostream&) const;

private:
    void emit(uint32_t);
    void emitPrologue(const std::function<void()>&);

    std::vector<uint32_t> m_instructions;
    std::vector<uint32_t>::iterator m_iterator;
};
