#include "Inspector.h"

#include "AST.h"
#include "Type.h"
#include "Value.h"
#include <iostream>

std::ostream& Inspector::stdout = std::cout;
std::ostream& Inspector::stderr = std::cerr;

void Inspector::dumpNode(const Node* expr)
{
    expr->dump(std::cerr, 0);
}

void Inspector::dumpType(const Type* type)
{
    std::cerr << *type << std::endl;
}

void Inspector::dumpValue(const Value& value)
{
    std::cerr << value << std::endl;
}
