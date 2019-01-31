import "Binding.h"

ast_node :ASTType < :Node,
    extra_methods: [
        "virtual const Binding& infer(TypeChecker&) = 0",
    ]

ast_node :ASTTypeType < :ASTType,
    extra_methods: [
        "virtual const Binding& infer(TypeChecker&)",
    ]
