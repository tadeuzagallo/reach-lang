#pragma once

#include "Cell.h"

class Type;

class Typed : public Cell {
public:
    CELL_TYPE(Typed)

    Type* type() const;

protected:
    Typed(Type*);

    void visit(const Visitor&) const override;

private:
    Type* m_type;
};
