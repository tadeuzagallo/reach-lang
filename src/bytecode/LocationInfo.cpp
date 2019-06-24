#include "LocationInfo.h"

void LocationInfoWithFile::dump(std::ostream& out) const
{
    out << filename << ":" << info.start.line << ":" << info.start.column;
}
