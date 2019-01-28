#include "Inspector.h"

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
