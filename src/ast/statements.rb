import "expressions.h"

import :memory
import :optional
import :variant
import :vector

declare :Declaration
declare :LexicalDeclaration

ast_node :Statement < :Node,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual const Type& infer(TypeChecker&)",
    "virtual void check(TypeChecker&, const Type&) = 0",
  ]

ast_node :EmptyStatement < :Statement,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Type&)",
  ]

ast_node :BlockStatement < :Statement,
  fields: {
    declarations: "std::vector<std::unique_ptr<Declaration>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Type&)",
  ]

ast_node :ReturnStatement < :Statement,
  fields: {
    expression: "std::optional<std::unique_ptr<Expression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Type&)",
  ]

ast_node :IfStatement < :Statement,
  fields: {
    condition: "std::unique_ptr<Expression>",
    consequent: "std::unique_ptr<Statement>",
    alternate: "std::optional<std::unique_ptr<Statement>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual const Type& infer(TypeChecker&)",
    "virtual void check(TypeChecker&, const Type&)",
  ]

ast_node :BreakStatement < :Statement,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Type&)",
  ]

ast_node :ContinueStatement < :Statement,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Type&)",
  ]

ast_node :WhileStatement < :Statement,
  fields: {
    condition: "std::unique_ptr<Expression>",
    body: "std::unique_ptr<Statement>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Type&)",
  ]

ast_node :ForStatement < :Statement,
  fields: {
    initializer: "std::optional<std::variant<std::unique_ptr<Expression>, std::unique_ptr<LexicalDeclaration>>>",
    condition: "std::optional<std::unique_ptr<Expression>>",
    increment: "std::optional<std::unique_ptr<Expression>>",
    body: "std::unique_ptr<Statement>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void check(TypeChecker&, const Type&)",
  ]

ast_node :ExpressionStatement < :Statement,
  fields: {
    expression: "std::unique_ptr<Expression>",
  },
  extra_methods: [
    "ExpressionStatement(std::unique_ptr<Expression>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual const Type& infer(TypeChecker&)",
    "virtual void check(TypeChecker&, const Type&)",
  ]
