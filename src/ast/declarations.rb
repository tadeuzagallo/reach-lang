import "BytecodeGenerator.h"
import "expressions.h"
import "statements.h"

import :memory
import :optional
import :vector

ast_node :Declaration < :Node,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register) = 0",
  ]

ast_node :LexicalDeclaration < :Declaration,
  fields: {
    name: "std::unique_ptr<Identifier>",
    type: "std::unique_ptr<Expression>",
    initializer: "std::unique_ptr<Expression>"
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :FunctionDeclaration < :Declaration,
  fields: {
    name: "std::unique_ptr<Identifier>",
    parameters: "std::vector<std::unique_ptr<TypedIdentifier>>",
    returnType: "std::unique_ptr<Expression>",
    body: "std::unique_ptr<BlockStatement>",
    functionIndex: "uint32_t",
  },
  extra_methods: [
    "void generateImpl(BytecodeGenerator&, Register)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :StatementDeclaration < :Declaration,
  fields: {
    statement: "std::unique_ptr<Statement>",
  },
  extra_methods: [
    "StatementDeclaration(std::unique_ptr<Statement>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]
