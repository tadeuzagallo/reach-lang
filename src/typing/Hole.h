#pragma once

#include "Array.h"
#include "Object.h"
#include "Register.h"
#include "RhString.h"
#include "Type.h"

class BytecodeGenerator;
class Environment;
class Identifier;
class TupleExpression;
class ObjectLiteralExpression;
class ArrayLiteralExpression;
class CallExpression;
class SubscriptExpression;
class MemberExpression;
class LiteralExpression;

class Hole : public Type {
public:
    CELL_TYPE(Hole);

    bool operator!=(const Hole& other) const;

    virtual bool operator==(const Hole&) const = 0;
    virtual void dump(std::ostream&) const = 0;
    virtual void generate(BytecodeGenerator&, Register) const = 0;
    virtual Hole* substitute(VM&, const Substitutions&) const = 0;
    virtual Value partiallyEvaluate(VM&, Environment*) = 0;

protected:
    Hole();
};

class HoleVariable : public Hole {
public:
    CELL_CREATE_VM(HoleVariable);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;
    void generate(BytecodeGenerator&, Register) const override;
    Hole* substitute(VM&, const Substitutions&) const override;
    Value partiallyEvaluate(VM&, Environment*) override;

private:
    HoleVariable(VM&, const std::string&);

    CELL_FIELD(String, name);
};

class HoleCall : public Hole {
public:
    CELL_CREATE_VM(HoleCall);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;
    void generate(BytecodeGenerator&, Register) const override;
    Hole* substitute(VM&, const Substitutions&) const override;
    Value partiallyEvaluate(VM&, Environment*) override;

private:
    HoleCall(VM&, Value, Array*);

    VALUE_FIELD(Value, callee);
    CELL_FIELD(Array, arguments);
};

class HoleSubscript : public Hole {
public:
    CELL_CREATE_VM(HoleSubscript);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;
    void generate(BytecodeGenerator&, Register) const override;
    Hole* substitute(VM&, const Substitutions&) const override;
    Value partiallyEvaluate(VM&, Environment*) override;

private:
    HoleSubscript(VM&, Value, Value);

    VALUE_FIELD(Value, target);
    VALUE_FIELD(Value, index);
};

class HoleMember : public Hole {
public:
    CELL_CREATE_VM(HoleMember);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;
    void generate(BytecodeGenerator&, Register) const override;
    Hole* substitute(VM&, const Substitutions&) const override;
    Value partiallyEvaluate(VM&, Environment*) override;

private:
    HoleMember(VM&, Value, String*);

    VALUE_FIELD(Value, object);
    CELL_FIELD(String, property);
};
