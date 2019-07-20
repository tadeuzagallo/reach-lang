import "BytecodeBlock.h"
import "VM.h"
import "declarations.h"
import :vector

ast_node :Program < :Node,
  fields: {
    declarations: "std::vector<std::unique_ptr<Declaration>>",
  },
  extra_methods: [
    "void dump(std::ostream&)",
    "void typecheck(BytecodeGenerator&)",
    "BytecodeBlock* generate(BytecodeGenerator&) const",
  ]
