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

    void link(uint32_t prologueSize, InstructionStream::Ref ref)
    {
        InstructionStream::Offset targetOffset = ref.offset();
        for (auto& it : m_references) {
            uint32_t prologueIncrease = prologueSize - it.second.prologueSize;
            it.second.ref += prologueIncrease;
            it.second.ref.write(targetOffset - it.second.ref.offset());
        }
    }

private:
    struct LabelReference {
        uint32_t prologueSize;
        InstructionStream::WritableRef ref;
    };

    void addReference(InstructionStream::Offset offset, uint32_t prologueSize, InstructionStream::WritableRef&& ref)
    {
        m_references.emplace(offset, LabelReference { prologueSize, std::move(ref) });
    }

    std::unordered_map<InstructionStream::Offset, LabelReference> m_references;
};
