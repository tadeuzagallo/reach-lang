import "literals.h"
import :vector
import :map
import :memory

ast_node :Expression < :Node,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual void infer(TypeChecker&, Register) = 0",
    "virtual void check(TypeChecker&, Register) = 0",
  ]

ast_node :Identifier < :Expression,
  fields: {
    name: "std::string",
    isOperator: "bool",
  },
  extra_methods: [
    "Identifier(const Token&, bool isOperator = false)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "bool operator<(const Identifier&) const",
    "friend std::ostream& operator<<(std::ostream&, const Identifier&)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :ParenthesizedExpression < :Expression,
  fields: {
    expression: "std::unique_ptr<Expression>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :TupleExpression < :Expression,
  fields: {
    items: "std::vector<std::unique_ptr<Expression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :ObjectLiteralExpression < :Expression,
  fields: {
    fields: "std::map<std::unique_ptr<Identifier>, std::unique_ptr<Expression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :ArrayLiteralExpression < :Expression,
  fields: {
    items: "std::vector<std::unique_ptr<Expression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :CallExpression < :Expression,
  fields: {
    callee: "std::unique_ptr<Expression>",
    arguments: "std::vector<std::unique_ptr<Expression>>",
  },
  extra_methods: [
    "CallExpression(std::unique_ptr<Expression>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
    "void checkCallee(TypeChecker&, Register, Label&)",
    "void checkArguments(TypeChecker&, Register, Label&)",
  ]

ast_node :SubscriptExpression < :Expression,
  fields: {
    target: "std::unique_ptr<Expression>",
    index: "std::unique_ptr<Expression>",
  },
  extra_methods: [
    "SubscriptExpression(std::unique_ptr<Expression>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :MemberExpression < :Expression,
  fields: {
    object: "std::unique_ptr<Expression>",
    property: "std::unique_ptr<Identifier>",
  },
  extra_methods: [
    "MemberExpression(std::unique_ptr<Expression>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :LiteralExpression < :Expression,
  fields: {
    literal: "std::unique_ptr<Literal>",
  },
  extra_methods: [
    "LiteralExpression(std::unique_ptr<Literal>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual void check(TypeChecker&, Register)",
  ]

ast_node :TypedIdentifier < :Node,
    fields: {
        name: "std::unique_ptr<Identifier>",
        type: "std::unique_ptr<Expression>",
        inferred: "bool",
    },
    extra_methods: [
        "virtual void infer(TypeChecker&, Register)",
    ]
