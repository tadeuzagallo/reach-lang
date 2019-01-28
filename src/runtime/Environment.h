#pragma once

#include <map>

class Identifier;
class Value;

class Environment {
    using Map = std::map<std::string, Value>;

public:
    Environment(const Environment* parent);

    void set(const Identifier& key, Value value);
    void set(const std::string& key, Value value);
    Value get(const Identifier& key) const;

    const Environment* parent() const;
    void visit(std::function<void(Value)>) const;

private:
    const Environment* m_parent;
    Map m_map;
};
