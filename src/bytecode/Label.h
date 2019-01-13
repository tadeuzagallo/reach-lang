#pragma once

#include "InstructionStream.h"
#include <unordered_map>

class Label {
    friend class InstructionStream;

public:
    Label() = default;

    // disable copying
    Label(const Label&) = delete;
    Label& operator=(const Label&) = delete;

    void link(InstructionStream::Ref ref)
    {
        for (auto& it : m_references) {
            it.second.write(ref.offset() - it.second.offset());
        }
    }

private:
    void addReference(InstructionStream::Offset offset, InstructionStream::WritableRef&& ref)
    {
        m_references.emplace(offset, std::move(ref));
    }

    std::unordered_map<InstructionStream::Offset, InstructionStream::WritableRef> m_references;
};
