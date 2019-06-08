ast_node :ASTType < :Node,
    extra_methods: [
        "virtual void infer(TypeChecker&, Register) = 0",
        "virtual void generate(BytecodeGenerator&, Register) = 0",
    ]

ast_node :ASTTypeType < :ASTType,
    extra_methods: [
        "virtual void infer(TypeChecker&, Register)",
        "virtual void generate(BytecodeGenerator&, Register)",
    ]
