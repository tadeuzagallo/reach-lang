#pragma once

#include "Cell.h"
#include "VM.h"
#include <map>

class Identifier;
class Value;

class Environment  : public Cell {
    using Map = std::map<std::string, Value>;

public:
    CELL(Environment);

    void set(const Identifier& key, Value value);
    void set(const std::string& key, Value value);
    Value get(const Identifier& key) const;

    const Environment* parent() const;
    void dump(std::ostream&) const override;
    void visit(std::function<void(Value)>) const override;

private:
    Environment(const Environment* parent);

    const Environment* m_parent;
    Map m_map;
};

extern Environment* createEnvironment(VM&, const Environment*);
