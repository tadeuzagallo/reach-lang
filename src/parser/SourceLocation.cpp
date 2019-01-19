#include "SourceLocation.h"

void SourceLocation::dump(std::ostream& out) const
{
    out << file.name << ":" << start.line << ":" << start.column;
}

std::ostream& operator<<(std::ostream& out, const SourceLocation& loc)
{
    loc.dump(out);
    return out;
}
