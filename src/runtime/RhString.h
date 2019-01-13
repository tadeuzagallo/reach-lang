#pragma once

#include "Cell.h"
#include "VM.h"

class String : public Cell {
public:
    CELL(String)

    void visit(std::function<void(Value)>) const override
    {
    }

    void dump(std::ostream& out) const override
    {
        out << std::string(m_data, m_length);
    }

private:
    String(const char* data, size_t length)
        : m_length(length)
        , m_data(data)
    {
    }

    size_t m_length;
    const char* m_data;
};
