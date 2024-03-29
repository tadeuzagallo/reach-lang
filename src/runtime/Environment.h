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
    Value get(const std::string& key, bool& success) const;

    Environment* parent() const;
    void dump(std::ostream&) const override;

protected:
    void visit(const Visitor&) const override;

private:
    Environment(Environment*);

    Environment* m_parent;
    Map m_map;
    Map m_typeMap;
};

// JIT helpers
extern "C" {

Environment* createEnvironment(VM&, Environment*);
int64_t jitEnvironmentGet(Environment*, const std::string&);
void jitUnknownVariable(VM&, uint32_t, const std::string&);

}
