#pragma once

#include <memory>

class Value;
class Type;
class Expression;

class Inspector {
static void dumpValue(const Value&);
static void dumpType(const Type&);
static void dumpExpression(const std::unique_ptr<Expression>&);
};
