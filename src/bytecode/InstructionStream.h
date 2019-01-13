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

    private:
        WritableRef(InstructionStream&, Offset, uint32_t);

        InstructionStream& m_instructions;
        Offset m_instructionOffset;
        uint32_t m_targetOffset;
    };

    Ref at(Offset) const;
    Ref begin() const;
    Ref end() const;

    template<typename JumpType, typename Label>
    void recordJump(Label& label)
    {
        Offset offset = m_instructions.size();
        label.addReference(offset, WritableRef { *this, offset, OFFSETOF(JumpType, target) >> 2 });
    }

    void dump(std::ostream&) const;

private:
    void emit(uint32_t);

    std::vector<uint32_t> m_instructions;
};
