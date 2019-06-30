#pragma once

#include "Cell.h"
#include "VM.h"

class String : public Cell {
public:
    CELL(String)

    const std::string& str() const { return m_str; }

    void visit(std::function<void(Value)>) const override
    {
    }

    void dump(std::ostream& out) const override
    {
        out << '"' << m_str << '"';
    }

    bool operator==(const String& other) const
    {
        return m_str == other.m_str;
    }

private:
    String(const std::string& str)
        : m_str(str)
    {
    }

    std::string m_str;
};
