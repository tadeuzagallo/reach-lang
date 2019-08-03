#include "Cell.h"

std::ostream& operator<<(std::ostream& out, Cell::Kind kind)
{
#define CASE(__kind) \
    case Cell::Kind::__kind: \
        out << "Cell::Kind::" #__kind; \
        return out;

    switch (kind) {
    CASE(Typed)
    CASE(Object)
    CASE(String)
    CASE(Array)
    CASE(Function)
    CASE(Tuple)
    CASE(Type)
    CASE(Hole)
    CASE(Environment)
    CASE(BytecodeBlock)
    }

#undef CASE
}
