#pragma once

#include "Typed.h"
#include "VM.h"

class String : public Typed {
public:
    CELL_TYPE(String)
    CELL_CREATE_VM(String)

    const std::string& str() const { return m_str; }

    void dump(std::ostream& out) const override
    {
        out << '"' << m_str << '"';
    }

    bool operator==(const String& other) const
    {
        return m_str == other.m_str;
    }

private:
    String(VM& vm, const std::string& str)
        : Typed(vm.stringType)
        , m_str(str)
    {
    }

    std::string m_str;
};
