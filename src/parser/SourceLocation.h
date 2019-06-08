#pragma once

#include <iostream>
#include <stddef.h>

struct SourceFile {
    const char* name;
    const char* source;
    size_t length;
};

struct SourcePosition {
    bool operator==(const SourcePosition& other) const
    {
        return line == other.line && column == other.column && offset == other.offset;
    }

    unsigned line;
    unsigned column;
    unsigned offset;
};

struct SourceLocation {
    SourceFile file;
    SourcePosition start;
    SourcePosition end;

    void dump(std::ostream&) const;
    friend std::ostream& operator<<(std::ostream&, const SourceLocation&);
};
