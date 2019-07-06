#pragma once

#include "Array.h"
#include "Object.h"
#include "RhString.h"

class Identifier;
class TupleExpression;
class ObjectLiteralExpression;
class ArrayLiteralExpression;
class CallExpression;
class SubscriptExpression;
class MemberExpression;
class LiteralExpression;

class Hole : public Object {
public:
    CELL_TYPE(Hole);

    bool operator!=(const Hole& other) const;
    virtual bool operator==(const Hole&) const = 0;
    virtual void dump(std::ostream&) const = 0;

protected:
    Hole();
};

class HoleIdentifier : public Hole {
public:
    CELL_CREATE_VM(HoleIdentifier);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;

private:
    HoleIdentifier(VM&, const Identifier&);

    CELL_FIELD(String, name);
};

class HoleTuple : public Hole {
public:
    CELL_CREATE_VM(HoleTuple);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;

private:
    HoleTuple(VM&, const TupleExpression&);

    CELL_FIELD(Array, items);
};

class HoleObject : public Hole {
public:
    CELL_CREATE_VM(HoleObject);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;

private:
    HoleObject(VM&, const ObjectLiteralExpression&);
};

class HoleArray : public Hole {
public:
    CELL_CREATE_VM(HoleArray);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;

private:
    HoleArray(VM&, const ArrayLiteralExpression&);

    CELL_FIELD(Array, items);
};

class HoleCall : public Hole {
public:
    CELL_CREATE_VM(HoleCall);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;

private:
    HoleCall(VM&, const CallExpression&);

    CELL_FIELD(Hole, callee);
    CELL_FIELD(Array, arguments);
};

class HoleSubscript : public Hole {
public:
    CELL_CREATE_VM(HoleSubscript);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;

private:
    HoleSubscript(VM&, const SubscriptExpression&);

    CELL_FIELD(Hole, target);
    CELL_FIELD(Hole, index);
};

class HoleMember : public Hole {
public:
    CELL_CREATE_VM(HoleMember);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;

private:
    HoleMember(VM&, const MemberExpression&);

    CELL_FIELD(Hole, object);
    CELL_FIELD(String, property);
};

class HoleLiteral : public Hole {
public:
    CELL_CREATE_VM(HoleLiteral);

    bool operator==(const Hole&) const override;
    void dump(std::ostream&) const override;

private:
    HoleLiteral(VM&, const LiteralExpression& literal);

    VALUE_FIELD(Value, value);
};
