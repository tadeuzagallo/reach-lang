#pragma once

#include "InstructionMacros.h"
#include <iostream>

struct Instruction {
protected:
    enum ID : uint32_t { INSTRUCTION_IDS };

public:
    using ID = ID;

private:
    static constexpr size_t count = INSTRUCTION_COUNT;
    static constexpr const char* names[] = { INSTRUCTION_NAMES };
    static constexpr size_t sizes[] = { INSTRUCTION_SIZES };

public:
    const char* name() const;
    size_t size() const;
    void dump(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& out, const Instruction& instruction)
    {
        instruction.dump(out);
        return out;
    }

    ID id;
};
