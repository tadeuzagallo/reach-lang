#pragma once

#include "Cell.h"

class Type;

class Typed : public Cell {
public:
    CELL_TYPE(Typed)

    Type* type() const;

    void visit(std::function<void(Value)>) const override;

protected:
    Typed(Type*);

private:
    Type* m_type;
};
