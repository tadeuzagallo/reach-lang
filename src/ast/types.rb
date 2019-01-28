import "expressions.h"
import "Binding.h"

ast_node :ASTType < :Node,
    extra_methods: [
        "virtual const Binding& normalize(TypeChecker&) = 0",
    ]

ast_node :ASTTypeType < :ASTType,
    extra_methods: [
        "virtual const Binding& normalize(TypeChecker&)",
    ]

ast_node :ASTTypeName < :ASTType,
    fields: {
        name: "std::unique_ptr<Identifier>",
    },
    extra_methods: [
        "virtual const Binding& normalize(TypeChecker&)",
    ]

ast_node :ASTTypeFunction < :ASTType,
    extra_methods: [
        "virtual const Binding& normalize(TypeChecker&)",
    ]

ast_node :TypedIdentifier < :Node,
    fields: {
        name: "std::unique_ptr<Identifier>",
        type: "std::unique_ptr<ASTType>",
    },
    extra_methods: [
        "virtual const Binding& normalize(TypeChecker&)",
    ]
