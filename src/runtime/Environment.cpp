#include "Environment.h"

#include "expressions.h"
#include "Value.h"

Environment::Environment(Environment* parent)
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

Value Environment::get(const std::string& key) const
{
    auto it = m_map.find(key);
    ASSERT(it != m_map.end() || m_parent, "Undefined variable: %s", key.c_str());
    if (it != m_map.end())
        return it->second;
    return m_parent->get(key);
}

void Environment::setType(const std::string& key, Value value)
{
    m_typeMap[key] = value;
}

Value Environment::getType(const std::string& key) const
{
    auto it = m_typeMap.find(key);
    ASSERT(it != m_typeMap.end() || m_parent, "Undefined variable: %s", key.c_str());
    if (it != m_typeMap.end())
        return it->second;
    return m_parent->getType(key);
}

Environment* Environment::parent() const
{
    return m_parent;
}

void Environment::visit(std::function<void(Value)> visitor) const
{
    for (const auto& pair : m_map)
        visitor(pair.second);
    for (const auto& pair : m_typeMap)
        visitor(pair.second);
}

void Environment::dump(std::ostream& out) const
{
    out << "Environment {" << std::endl;
    for (const auto& pair : m_map) {
        out << pair.first << ": ";
        pair.second.dump(out);
        out << std::endl;
    }
    out << "}";
}

Environment* createEnvironment(VM& vm, Environment* parentEnvironment)
{
    return Environment::create(vm, parentEnvironment);
}
