#include "Object.h"

#include "BytecodeBlock.h"

Object::Object(Type* type, const BytecodeBlock& block, uint32_t fieldCount, const Value* keys, const Value* values)
    : Typed(type)
{
    for (uint32_t i = 0; i < fieldCount; i++) {
        const std::string& key = block.identifier(keys[i].asNumber());
        m_fields.emplace(key, values[i]);
    }
}

void Object::visit(const Visitor& visitor) const
{
    Typed::visit(visitor);
    for (auto field : m_fields)
        visitor.visit(field.second);
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

Object* createObject(VM& vm, Type* type, uint32_t inlineSize)
{
    return Object::create(vm, type, inlineSize);
}

int64_t tryGetJIT(Object* object, const std::string& field)
{
    return object->tryGet(field).value_or(Value::crash()).bits();
}
