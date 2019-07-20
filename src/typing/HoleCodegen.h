#pragma once

#include "Value.h"

class BytecodeBlock;
class BytecodeGenerator;

BytecodeBlock* holeCodegen(Value, BytecodeGenerator&);
