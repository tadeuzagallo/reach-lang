#pragma once

#include "SourceLocation.h"
#include <iostream>

struct LocationInfo {
    uint32_t bytecodeOffset;
    SourcePosition start;
    SourcePosition end;
};

struct LocationInfoWithFile {
    void dump(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& out, const LocationInfoWithFile& info)
    {
        info.dump(out);
        return out;
    }

    const char* filename;
    const LocationInfo& info;
};

