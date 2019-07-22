import "statements.h"

ast_node :Pattern < :Node,
    extra_methods: [
        "virtual void generate(BytecodeGenerator&, Register, Label&) = 0",
        "virtual void infer(TypeChecker&, Register) = 0",
    ]

ast_node :IdentifierPattern < :Pattern,
    fields: {
        name: "std::unique_ptr<Identifier>",
    },
    extra_methods: [
        "virtual void generate(BytecodeGenerator&, Register, Label&)",
        "virtual void infer(TypeChecker&, Register)",
    ]

ast_node :ObjectPattern < :Pattern,
    fields: {
        entries: "std::unordered_map<std::unique_ptr<Identifier>, std::unique_ptr<Pattern>>",
    },
    extra_methods: [
        "virtual void generate(BytecodeGenerator&, Register, Label&)",
        "virtual void infer(TypeChecker&, Register)",
    ]

ast_node :UnderscorePattern < :Pattern,
    extra_methods: [
        "virtual void generate(BytecodeGenerator&, Register, Label&)",
        "virtual void infer(TypeChecker&, Register)",
    ]

ast_node :LiteralPattern < :Pattern,
    fields: {
        literal: "std::unique_ptr<Literal>",
    },
    extra_methods: [
        "virtual void generate(BytecodeGenerator&, Register, Label&)",
        "virtual void infer(TypeChecker&, Register)",
    ]

ast_node :MatchCase < :Node,
    fields: {
        pattern: "std::unique_ptr<Pattern>",
        statement: "std::unique_ptr<Statement>",
    },
    extra_methods: [
    ]

ast_node :MatchStatement < :Statement,
    fields: {
        scrutinee: "std::unique_ptr<InferredExpression>",
        cases: "std::vector<std::unique_ptr<MatchCase>>",
        defaultCase: "std::unique_ptr<Statement>"
    },
    extra_methods: [
        "virtual void generate(BytecodeGenerator&, Register)",
        "virtual void infer(TypeChecker&, Register)",
        "virtual void check(TypeChecker&, Register)",
    ]
