#include "LocationInfo.h"

void LocationInfoWithFile::dump(std::ostream& out) const
{
    if (!filename)
        return;
    out << filename << ":" << info.start.line << ":" << info.start.column;
}
