#include "Inspector.h"

#include "AST.h"
#include "Type.h"
#include "Value.h"
#include <iostream>

void Inspector::dumpValue(const Value& value)
{
    std::cerr << value << std::endl;
}

void Inspector::dumpType(const Type& type)
{
    std::cerr << type << std::endl;
}

void Inspector::dumpExpression(const std::unique_ptr<Expression>& expr)
{
    expr->dump(std::cerr, 0);
}
