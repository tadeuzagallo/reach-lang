import "expressions.h"

import :memory
import :vector

ast_node :TypeExpression < :InferredExpression,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual void infer(TypeChecker&, Register) = 0",
  ]

ast_node :TupleTypeExpression < :TypeExpression,
  fields: {
    items: "std::vector<std::unique_ptr<InferredExpression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void generateForTypeChecking(TypeChecker&, Register)",
    "virtual void infer(TypeChecker&, Register)",
  ]

ast_node :FunctionTypeExpression < :TypeExpression,
  fields: {
    parameters: "std::vector<std::unique_ptr<InferredExpression>>",
    returnType: "std::unique_ptr<InferredExpression>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void generateForTypeChecking(TypeChecker&, Register)",
    "virtual void infer(TypeChecker&, Register)",
  ]

ast_node :UnionTypeExpression < :TypeExpression,
  fields: {
    lhs: "std::unique_ptr<InferredExpression>",
    rhs: "std::unique_ptr<InferredExpression>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void generateForTypeChecking(TypeChecker&, Register)",
    "virtual void infer(TypeChecker&, Register)",
  ]

ast_node :ObjectTypeExpression < :TypeExpression,
  fields: {
    fields: "std::map<std::unique_ptr<Identifier>, std::unique_ptr<InferredExpression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void generateForTypeChecking(TypeChecker&, Register)",
    "virtual void infer(TypeChecker&, Register)",
  ]

ast_node :ArrayTypeExpression < :TypeExpression,
  fields: {
    itemType: "std::unique_ptr<InferredExpression>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void generateForTypeChecking(TypeChecker&, Register)",
    "virtual void infer(TypeChecker&, Register)",
  ]

ast_node :SynthesizedTypeExpression < :TypeExpression,
    fields: {
        typeIndex: "uint32_t",
    },
    extra_methods: [
        "virtual void generate(BytecodeGenerator&, Register)",
        "virtual void generateForTypeChecking(TypeChecker&, Register)",
        "virtual void infer(TypeChecker&, Register)",
    ]

ast_node :TypeTypeExpression < :TypeExpression,
    extra_methods: [
        "virtual void generate(BytecodeGenerator&, Register)",
        "virtual void generateForTypeChecking(TypeChecker&, Register)",
        "virtual void infer(TypeChecker&, Register)",
    ]
