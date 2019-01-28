#include "Environment.h"

#include "expressions.h"
#include "Value.h"

Environment::Environment(const Environment* parent)
    : m_parent(parent)
{
}

void Environment::set(const Identifier& key, Value value)
{
    set(key.name, value);
}

void Environment::set(const std::string& key, Value value)
{
    m_map[key] = value;
}

Value Environment::get(const Identifier& key) const
{
    auto it = m_map.find(key.name);
    ASSERT(it != m_map.end() || m_parent, "Undefined variable: %s", key.name.c_str());
    if (it != m_map.end())
        return it->second;
    return m_parent->get(key);
}

const Environment* Environment::parent() const
{
    return m_parent;
}

void Environment::visit(std::function<void(Value)> visitor) const
{
    for (const auto& pair : m_map) {
        visitor(pair.second);
    }
}
