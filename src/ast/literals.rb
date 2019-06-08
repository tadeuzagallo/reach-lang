import :string
import :typeinfo

ast_node :Literal < :Node,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual void infer(TypeChecker&, Register) = 0",
  ]

ast_node :BooleanLiteral < :Literal,
  fields: {
    value: "bool"
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
  ]

ast_node :NumericLiteral < :Literal,
  fields: {
    value: "double",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
  ]

ast_node :StringLiteral < :Literal,
  fields: {
    value: "std::string",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual void infer(TypeChecker&, Register)",
  ]
