#pragma once

#include <memory>

#include "AST.h"

class Lexer;

class Parser {
    enum class IsTopLevel { No, Yes };

public:
    Parser(Lexer&);

    std::unique_ptr<Program> parse();

    std::unique_ptr<TypedIdentifier> parseTypedIdentifier(const Token&);

    std::unique_ptr<Declaration> parseDeclaration(const Token&);
    std::unique_ptr<LexicalDeclaration> parseLexicalDeclaration(const Token&);
    std::unique_ptr<FunctionDeclaration> parseFunctionDeclaration(const Token&);

    std::unique_ptr<Statement> parseStatement(const Token&, IsTopLevel = IsTopLevel::No);
    std::unique_ptr<BlockStatement> parseBlockStatement(const Token&);
    std::unique_ptr<IfStatement> parseIfStatement(const Token& t);
    std::unique_ptr<ForStatement> parseForStatement(const Token& t);
    std::unique_ptr<WhileStatement> parseWhileStatement(const Token& t);
    std::unique_ptr<ReturnStatement> parseReturnStatement(const Token& t);

    std::unique_ptr<Expression> parseExpression(const Token&);

    std::unique_ptr<Expression> parseSuffixExpression(std::unique_ptr<Expression>, bool*);
    std::unique_ptr<CallExpression> parseCallExpression(std::unique_ptr<Expression>);
    std::unique_ptr<Expression> parseSubscriptExpression(std::unique_ptr<Expression>);
    std::unique_ptr<Expression> parseMemberExpression(std::unique_ptr<Expression>);
    std::unique_ptr<Expression> parseBinaryExpression(std::unique_ptr<Expression>, bool*);
    std::unique_ptr<FunctionTypeExpression> parseFunctionTypeExpression(std::unique_ptr<Expression>);

    std::unique_ptr<Expression> parsePrimaryExpression(const Token&);
    std::unique_ptr<Identifier> parseIdentifier(const Token&);
    std::unique_ptr<Identifier> parseOperator(const Token&);
    std::unique_ptr<ArrayLiteralExpression> parseArrayLiteralExpression(const Token&);
    std::unique_ptr<TypeExpression> parseTypeExpression(const Token&);
    std::unique_ptr<Expression> parseObjectLiteralExpression(const Token&);
    std::unique_ptr<Expression> parseParenthesizedExpressionOrTuple(const Token&);
    std::unique_ptr<TupleTypeExpression> parseTupleTypeExpression(const Token&);

    std::unique_ptr<Literal> parseLiteral(const Token&);
    std::unique_ptr<NumericLiteral> parseNumericLiteral(const Token&);
    std::unique_ptr<StringLiteral> parseStringLiteral(const Token&);
    std::unique_ptr<BooleanLiteral> parseBooleanLiteral(const Token&, bool);

    std::unique_ptr<ASTType> parseType(const Token&);
    std::unique_ptr<ASTTypeType> parseTypeType(const Token&);

    void parseError(const Token&, const std::string&);
    void unexpectedToken(const Token&);
    void unexpectedToken(const Token&, Token::Type);
    void reportErrors(std::ostream&);

private:
    class Error {
    public:
        Error(const SourceLocation&, const std::string&);

        const SourceLocation& location() const;
        const std::string& message() const;

    private:
        SourceLocation m_location;
        std::string m_message;
    };

    class EndsWith;
    friend EndsWith;

    Token::Type m_endsWithToken { Token::UNKNOWN };
    Lexer& m_lexer;
    std::vector<Error> m_errors;
};
