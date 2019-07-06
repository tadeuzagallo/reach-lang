import "literals.h"
import :vector
import :map
import :memory

ast_node :Expression < :Node,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",

    "private: Expression(const Token&)",
    "friend class CheckedExpression",
  ]

ast_node :CheckedExpression < :Expression,
  extra_methods: [
    "CheckedExpression(const Token&)",
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual void check(TypeChecker&, Register) = 0",
    "virtual Hole* asHole(VM&) = 0",
  ]

ast_node :InferredExpression < :CheckedExpression,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual void infer(TypeChecker&, Register) = 0",
    "virtual void check(TypeChecker&, Register) final",
  ]

ast_node :Identifier < :InferredExpression,
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
    "virtual Hole* asHole(VM&)",
  ]

ast_node :ParenthesizedExpression < :InferredExpression,
  fields: {
    expression: "std::unique_ptr<InferredExpression>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual Hole* asHole(VM&)",
  ]

ast_node :TupleExpression < :InferredExpression,
  fields: {
    items: "std::vector<std::unique_ptr<InferredExpression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual Hole* asHole(VM&)",
  ]

ast_node :ObjectLiteralExpression < :InferredExpression,
  fields: {
    fields: "std::map<std::unique_ptr<Identifier>, std::unique_ptr<InferredExpression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual Hole* asHole(VM&)",
  ]

ast_node :ArrayLiteralExpression < :InferredExpression,
  fields: {
    items: "std::vector<std::unique_ptr<InferredExpression>>",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual Hole* asHole(VM&)",
  ]

ast_node :CallExpression < :InferredExpression,
  fields: {
    callee: "std::unique_ptr<InferredExpression>",
    arguments: "std::vector<std::unique_ptr<CheckedExpression>>",
  },
  extra_methods: [
    "CallExpression(std::unique_ptr<InferredExpression>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "void checkCallee(TypeChecker&, Register, Label&)",
    "void checkArguments(TypeChecker&, Register, Label&)",
    "virtual Hole* asHole(VM&)",
  ]

ast_node :SubscriptExpression < :InferredExpression,
  fields: {
    target: "std::unique_ptr<InferredExpression>",
    index: "std::unique_ptr<CheckedExpression>",
  },
  extra_methods: [
    "SubscriptExpression(std::unique_ptr<InferredExpression>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual Hole* asHole(VM&)",
  ]

ast_node :MemberExpression < :InferredExpression,
  fields: {
    object: "std::unique_ptr<InferredExpression>",
    property: "std::unique_ptr<Identifier>",
  },
  extra_methods: [
    "MemberExpression(std::unique_ptr<InferredExpression>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual Hole* asHole(VM&)",
  ]

ast_node :LiteralExpression < :InferredExpression,
  fields: {
    literal: "std::unique_ptr<Literal>",
  },
  extra_methods: [
    "LiteralExpression(std::unique_ptr<Literal>)",
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
    "virtual Hole* asHole(VM&)",
  ]
