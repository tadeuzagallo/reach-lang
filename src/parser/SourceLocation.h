#pragma once

#include <stddef.h>

struct SourceFile {
    const char* name;
    const char* source;
    size_t length;
};

struct SourcePosition {
    unsigned line;
    unsigned column;
    unsigned offset;
};

struct SourceLocation {
    SourceFile file;
    SourcePosition start;
    SourcePosition end;
};
