#pragma once

#include <iostream>
#include <memory>

class Node;
class Type;
class Value;

class Inspector {
static std::ostream& stdout;
static std::ostream& stderr;

static void dumpNode(const Node*);
static void dumpType(const Type*);
static void dumpValue(const Value&);
};
