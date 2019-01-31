import "BytecodeGenerator.h"
import "expressions.h"
import "statements.h"
import "types.h"

import :memory
import :optional
import :vector

ast_node :Declaration < :Node,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual const Binding& infer(TypeChecker&)",
    "virtual void check(TypeChecker&, const Binding&) = 0",
  ]

ast_node :LexicalDeclaration < :Declaration,
  fields: {
    isConst: "bool",
    name: "std::unique_ptr<Identifier>",
    initializer: "std::optional<std::unique_ptr<Expression>>"
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Binding&)",
  ]

ast_node :FunctionDeclaration < :Declaration,
  fields: {
    name: "std::unique_ptr<Identifier>",
    parameters: "std::vector<std::unique_ptr<TypedIdentifier>>",
    returnType: "std::unique_ptr<Expression>",
    body: "std::unique_ptr<BlockStatement>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Binding&)",
  ]

ast_node :StatementDeclaration < :Declaration,
  fields: {
    statement: "std::unique_ptr<Statement>",
  },
  extra_methods: [
    "StatementDeclaration(std::unique_ptr<Statement>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual const Binding& infer(TypeChecker&)",
    "virtual void check(TypeChecker&, const Binding&)",
  ]
