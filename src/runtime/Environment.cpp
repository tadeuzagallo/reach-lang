#include "Environment.h"

#include "expressions.h"
#include "Value.h"
#include <sstream>

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

Value Environment::get(const std::string& key, bool& success) const
{
    auto it = m_map.find(key);
    if (it == m_map.end() && !m_parent) {
        success = false;
        return Value::unit();
    }

    success = true;
    if (it != m_map.end())
        return it->second;
    return m_parent->get(key, success);
}

Environment* Environment::parent() const
{
    return m_parent;
}

void Environment::visit(std::function<void(Value)> visitor) const
{
    visitor(m_parent);
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

// JIT helpers
Environment* createEnvironment(VM& vm, Environment* parentEnvironment)
{
    return Environment::create(vm, parentEnvironment);
}

int64_t jitEnvironmentGet(Environment* env, const std::string& variable)
{
    bool success;
    Value value = env->get(variable, success);
    if (!success)
        value = Value::crash();
    return value.bits();
}

void jitUnknownVariable(VM& vm, uint32_t bytecodeOffset, const std::string& variable)
{
        std::stringstream message;
        message << "Unknown variable: `" << variable << "`";
        vm.typeError(bytecodeOffset, message.str());
}
