import :string
import :typeinfo

ast_node :Literal < :Node,
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register) = 0",
    "virtual const Type& infer(TypeChecker&) = 0",
  ]

ast_node :BooleanLiteral < :Literal,
  fields: {
    value: "bool"
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual const Type& infer(TypeChecker&)",
  ]

ast_node :NumericLiteral < :Literal,
  fields: {
    value: "double",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual const Type& infer(TypeChecker&)",
  ]

ast_node :StringLiteral < :Literal,
  fields: {
    value: "std::string",
  },
  extra_methods: [
    "virtual void generate(BytecodeGenerator&, Register)",
    "virtual const Type& infer(TypeChecker&)",
  ]
