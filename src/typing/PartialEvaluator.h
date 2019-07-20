#pragma once

#include "Value.h"

class Environment;
class VM;

Value partiallyEvaluate(Value, VM&, Environment*);
