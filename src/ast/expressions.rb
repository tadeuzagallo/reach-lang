import "literals.h"
import :vector
import :map
import :memory

ast_node :Expression < :Node,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0"
  ]

ast_node :Identifier < :Expression,
  fields: {
    name: "std::string",
  },
  extra_methods: [
    "Identifier(const Token&)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "bool operator<(const Identifier&) const",
    "friend std::ostream& operator<<(std::ostream&, const Identifier&)"
  ]

ast_node :BinaryExpression < :Expression,
  fields: {
    lhs: "std::unique_ptr<Expression>",
    op: "std::unique_ptr<Identifier>",
    rhs: "std::unique_ptr<Expression>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)"
  ]

ast_node :ThisExpression < :Expression,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)"
  ]
  
ast_node :ParenthesizedExpression < :Expression,
  fields: {
    expression: "std::unique_ptr<Expression>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)"
  ]

ast_node :ObjectLiteralExpression < :Expression,
  fields: {
    fields: "std::map<std::unique_ptr<Identifier>, std::unique_ptr<Expression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)"
  ]

ast_node :ArrayLiteralExpression < :Expression,
  fields: {
    items: "std::vector<std::unique_ptr<Expression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)"
  ]

ast_node :CallExpression < :Expression,
  fields: {
    callee: "std::unique_ptr<Expression>",
    arguments: "std::vector<std::unique_ptr<Expression>>",
  },
  extra_methods: [
    "CallExpression(std::unique_ptr<Expression>)",
    "virtual void generate(BytecodeGenerator&, Register)"
  ]

ast_node :SubscriptExpression < :Expression,
  fields: {
    target: "std::unique_ptr<Expression>",
    index: "std::unique_ptr<Expression>",
  },
  extra_methods: [
    "SubscriptExpression(std::unique_ptr<Expression>)",
    "virtual void generate(BytecodeGenerator&, Register)"
  ]

ast_node :MemberExpression < :Expression,
  fields: {
    object: "std::unique_ptr<Expression>",
    property: "std::unique_ptr<Identifier>",
  },
  extra_methods: [
    "MemberExpression(std::unique_ptr<Expression>)",
    "virtual void generate(BytecodeGenerator&, Register)"
  ]

ast_node :MethodCallExpression < :Expression,
  fields: {
    target: "std::unique_ptr<Expression>",
    call: "std::unique_ptr<CallExpression>",
  },
  extra_methods: [
    "MethodCallExpression(std::unique_ptr<Expression>)",
    "virtual void generate(BytecodeGenerator&, Register)"
  ]

ast_node :LiteralExpression < :Expression,
  fields: {
    literal: "std::unique_ptr<Literal>",
  },
  extra_methods: [
    "LiteralExpression(std::unique_ptr<Literal>)",
    "virtual void generate(BytecodeGenerator&, Register)"
  ]
