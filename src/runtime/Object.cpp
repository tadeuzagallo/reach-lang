#include "Object.h"

#include "BytecodeBlock.h"

Object::Object(const BytecodeBlock& block, uint32_t fieldCount, const Value* keys, const Value* values)
{
    for (uint32_t i = 0; i < fieldCount; i++) {
        const std::string& key = block.identifier(keys[i].asNumber());
        m_fields.emplace(key, values[i]);
    }
}

void Object::visit(std::function<void(Value)> visitor) const
{
    for (auto field : m_fields)
        visitor(field.second);
}

void Object::dump(std::ostream& out) const
{
    out << "{";
    bool first = true;
    for (const auto& field : m_fields) {
        if (!first)
            out << ", ";
        out << field.first << " = " << field.second;
        first = false;
    }
    out << "}";
}

Object* createObject(VM& vm, uint32_t inlineSize)
{
    return Object::create(vm, inlineSize);
}
