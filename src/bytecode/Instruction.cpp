#include "Instruction.h"

#include "Instructions.h"

const char* Instruction::name() const
{
  return names[id];
}

size_t Instruction::size() const
{
  return sizes[id];
}

void Instruction::dump(std::ostream& out) const
{
#define CASE(__Instruction) \
    case __Instruction::ID: \
        reinterpret_cast<const struct __Instruction*>(this)->dump(out); \
        break;

    out << name() << "(";
    switch (id) {
    FOR_EACH_INSTRUCTION(CASE)
    }
    out << ")";

#undef CASE
}
