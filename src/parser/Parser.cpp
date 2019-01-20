#include "Parser.h"

#include "Assert.h"
#include "Lexer.h"
#include <sstream>

#define CHECK(__token, __type) \
    do { \
        auto __t = (__token); \
        if (__t.type != __type) { \
            unexpectedToken(__t, __type); \
            return nullptr; \
        } \
    } while (false)

#define CONSUME(__type) CHECK(m_lexer.next(), __type)

template<typename T, typename U>
static std::unique_ptr<T> wrap(std::unique_ptr<U> wrapped)
{
    if (!wrapped)
        return nullptr;
    return std::make_unique<T>(std::move(wrapped));
}

Parser::Parser(Lexer& lexer)
    : m_lexer(lexer)
{ }

std::unique_ptr<Program> Parser::parse()
{
    auto t = m_lexer.next();
    auto program = std::make_unique<Program>(t);

    while (t.type != Token::END_OF_FILE) {
        program->declarations.emplace_back(parseDeclaration(t));
        t = m_lexer.next();
    }
    CHECK(t, Token::END_OF_FILE);

    if (m_errors.size())
        return nullptr;

    if (std::getenv("DUMP_AST"))
        program->dump(std::cout);

    return program;
}

std::unique_ptr<Declaration> Parser::parseDeclaration(const Token& t)
{
    switch (t.type) {
    case Token::LET:
    case Token::CONST:
        return parseLexicalDeclaration(t);
    case Token::FUNCTION:
        return parseFunctionDeclaration(t);
    default:
        return wrap<StatementDeclaration>(parseStatement(t));
    };
}

std::unique_ptr<LexicalDeclaration> Parser::parseLexicalDeclaration(const Token& t)
{
    auto decl = std::make_unique<LexicalDeclaration>(t);
    decl->name = parseIdentifier(m_lexer.next());

    if (m_lexer.peek().type == Token::EQUAL) {
        m_lexer.next();
        decl->initializer = parseExpression(m_lexer.next());
    }

    //ASSERT(m_lexer.peek().type == Token::SEMICOLON, "Expected semicolon after lexical declaration");
    m_lexer.next();

    return decl;
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration(const Token& t)
{
    CHECK(t, Token::FUNCTION);

    auto fn = std::make_unique<FunctionDeclaration>(t);
    fn->name = parseIdentifier(m_lexer.next());

    CONSUME(Token::L_PAREN);
    while (m_lexer.peek().type != Token::R_PAREN) {
        fn->parameters.emplace_back(parseTypedIdentifier(m_lexer.next()));
        if (m_lexer.peek().type == Token::COMMA) m_lexer.next();
        else
            break;
    }
    CONSUME(Token::R_PAREN);
    CONSUME(Token::ARROW);
    fn->returnType = parseType(m_lexer.next());
    fn->body = parseBlockStatement(m_lexer.next());

    return fn;
}

std::unique_ptr<Statement> Parser::parseStatement(const Token& t)
{
    switch (t.type) {
    case Token::L_BRACE:
        return parseBlockStatement(t);
    case Token::IF:
        return parseIfStatement(t);
    case Token::FOR:
        return parseForStatement(t);
    case Token::WHILE:
        return parseWhileStatement(t);
    case Token::RETURN:
        return parseReturnStatement(t);
    case Token::SEMICOLON:
        return std::make_unique<EmptyStatement>(t);
    default:
        auto expr = wrap<ExpressionStatement>(parseExpression(t));
        //ASSERT(m_lexer.peek().type == Token::SEMICOLON, "Expected semicolon after lexical expression");
        //m_lexer.next();
        return expr;
    }
}

std::unique_ptr<BlockStatement> Parser::parseBlockStatement(const Token& t)
{
    auto block = std::make_unique<BlockStatement>(t);

    CHECK(t, Token::L_BRACE);
    while (m_lexer.peek().type != Token::R_BRACE)
        block->declarations.emplace_back(parseDeclaration(m_lexer.next()));
    CONSUME(Token::R_BRACE);

    return block;
}

std::unique_ptr<IfStatement> Parser::parseIfStatement(const Token& t)
{
    auto ifStmt = std::make_unique<IfStatement>(t);

    CHECK(t, Token::IF);
    CONSUME(Token::L_PAREN);
    ifStmt->condition = parseExpression(m_lexer.next());
    CONSUME(Token::R_PAREN);

    ifStmt->consequent = parseStatement(m_lexer.next());

    if (m_lexer.peek().type == Token::ELSE) {
        CONSUME(Token::ELSE);
        ifStmt->alternate = parseStatement(m_lexer.next());
    }

    return ifStmt;
}

std::unique_ptr<ForStatement> Parser::parseForStatement(const Token& t)
{
    CHECK(t, Token::FOR);
    return nullptr;
}

std::unique_ptr<WhileStatement> Parser::parseWhileStatement(const Token& t)
{
    CHECK(t, Token::WHILE);
    return nullptr;
}

std::unique_ptr<ReturnStatement> Parser::parseReturnStatement(const Token& t)
{
    CHECK(t, Token::RETURN);
    auto ret = std::make_unique<ReturnStatement>(t);
    auto next = m_lexer.next();
    //if (next.type == Token::SEMICOLON)
    //return ret;

    ret->expression = parseExpression(next);
    //ASSERT(m_lexer.next().type == Token::SEMICOLON, "Expected semicolon after return statement");
    return ret;
}

std::unique_ptr<Expression> Parser::parseExpression(const Token& t)
{
    auto expr = parsePrimaryExpression(t);
    bool stop = false;
    while (expr && !stop)
        expr = parseSuffixExpression(std::move(expr), &stop);
    return expr;
}

std::unique_ptr<Expression> Parser::parseSuffixExpression(std::unique_ptr<Expression> expr, bool* stop)
{
    switch (m_lexer.peek().type) {
    case Token::L_PAREN:
        return parseCallExpression(std::move(expr));
    case Token::L_SQUARE:
        return parseSubscriptExpression(std::move(expr));
    case Token::DOT:
        return parseMemberExpression(std::move(expr));
    case Token::ARROW:
        return parseMethodCallExpression(std::move(expr));
    default:
        *stop = true;
        return expr;
    }
}

std::unique_ptr<CallExpression> Parser::parseCallExpression(std::unique_ptr<Expression> callee)
{
    CONSUME(Token::L_PAREN);
    auto call = std::make_unique<CallExpression>(std::move(callee));
    while (m_lexer.peek().type != Token::R_PAREN) {
        call->arguments.emplace_back(parseExpression(m_lexer.next()));
        if (m_lexer.peek().type == Token::COMMA)
            m_lexer.next();
        else
            break;
    }
    CONSUME(Token::R_PAREN);

    return call;
}

std::unique_ptr<SubscriptExpression> Parser::parseSubscriptExpression(std::unique_ptr<Expression> target)
{
    CONSUME(Token::L_SQUARE);
    auto subscript = std::make_unique<SubscriptExpression>(std::move(target));
    subscript->index = parseExpression(m_lexer.next());
    CONSUME(Token::R_SQUARE);

    return subscript;
}

std::unique_ptr<MemberExpression> Parser::parseMemberExpression(std::unique_ptr<Expression> object)
{
    CONSUME(Token::DOT);
    auto expr = std::make_unique<MemberExpression>(std::move(object));
    expr->property = parseIdentifier(m_lexer.next());
    return expr;
}

std::unique_ptr<MethodCallExpression> Parser::parseMethodCallExpression(std::unique_ptr<Expression> object)
{
    CONSUME(Token::ARROW);
    auto expr = std::make_unique<MethodCallExpression>(std::move(object));
    std::unique_ptr<Identifier> callee = parseIdentifier(m_lexer.next());
    expr->call = parseCallExpression(std::move(callee));
    return expr;
}

std::unique_ptr<Expression> Parser::parsePrimaryExpression(const Token &t)
{
    switch (t.type) {
    case Token::L_SQUARE:
        return parseArrayLiteralExpression(t);
    case Token::L_BRACE:
        return parseObjectLiteralExpression(t);
    //case Token::L_PAREN:
        //return parseParenthesizedExpression(t);
    //case Token::FUNCTION:
        //return parseFunctionExpression(t);
    case Token::IDENTIFIER:
        return parseIdentifier(t);
    default:
        return wrap<LiteralExpression>(parseLiteral(t));
    }
}

std::unique_ptr<ArrayLiteralExpression> Parser::parseArrayLiteralExpression(const Token& t)
{
    CHECK(t, Token::L_SQUARE);
    auto array = std::make_unique<ArrayLiteralExpression>(t);
    Token tok = m_lexer.next();
    while (tok.type != Token::R_SQUARE) {
        array->items.push_back(parseExpression(tok));
        tok = m_lexer.next();
        if (tok.type == Token::COMMA)
            tok = m_lexer.next();
        else
            break;
    }
    CHECK(tok, Token::R_SQUARE);
    return array;
}

std::unique_ptr<ObjectLiteralExpression> Parser::parseObjectLiteralExpression(const Token& t)
{
    CHECK(t, Token::L_BRACE);
    auto record = std::make_unique<ObjectLiteralExpression>(t);
    Token tok = m_lexer.next();
    while (tok.type != Token::R_BRACE) {
        auto name = parseIdentifier(tok);
        CONSUME(Token::EQUAL);
        auto value = parseExpression(m_lexer.next());
        record->fields[std::move(name)] = std::move(value);
        tok = m_lexer.next();
        if (tok.type == Token::COMMA)
            tok = m_lexer.next();
        else
            break;
    }
    CHECK(tok, Token::R_BRACE);
    return record;
}

std::unique_ptr<Identifier> Parser::parseIdentifier(const Token& t)
{
    CHECK(t, Token::IDENTIFIER);
    return std::make_unique<Identifier>(t);
}

std::unique_ptr<TypedIdentifier> Parser::parseTypedIdentifier(const Token& t)
{
    auto typedIdentifier = std::make_unique<TypedIdentifier>(t);
    typedIdentifier->name = parseIdentifier(t);
    CONSUME(Token::COLON);
    typedIdentifier->type = parseType(m_lexer.next());
    return typedIdentifier;
}

std::unique_ptr<Literal> Parser::parseLiteral(const Token& t)
{
    switch (t.type) {
    case Token::NUMBER:
        return parseNumericLiteral(t);
    case Token::STRING:
        return parseStringLiteral(t);
    case Token::TRUE:
        return parseBooleanLiteral(t, true);
    case Token::FALSE:
        return parseBooleanLiteral(t, false);
    default:
        unexpectedToken(t);
        return nullptr;
    }
}

std::unique_ptr<NumericLiteral> Parser::parseNumericLiteral(const Token& t)
{
    auto num = std::make_unique<NumericLiteral>(t);
    num->value = strtod(t.lexeme().c_str(), nullptr);
    return num;
}

std::unique_ptr<StringLiteral> Parser::parseStringLiteral(const Token& t)
{
    auto str = std::make_unique<StringLiteral>(t);
    str->location = t.location;
    str->value = t.lexeme();
    return str;
}

std::unique_ptr<BooleanLiteral> Parser::parseBooleanLiteral(const Token& t, bool value)
{
    auto boolean = std::make_unique<BooleanLiteral>(t);
    boolean->value = value;
    return boolean;
}

// Types
std::unique_ptr<ASTType> Parser::parseType(const Token& t)
{
    switch (t.type) {
    case Token::IDENTIFIER:
        return parseTypeName(t);
    default:
        unexpectedToken(t);
        return nullptr;
    };
}

std::unique_ptr<ASTTypeName> Parser::parseTypeName(const Token& t)
{
    auto typeName = std::make_unique<ASTTypeName>(t);
    typeName->name = parseIdentifier(t);
    return typeName;
}

// Error handling
Parser::Error::Error(const SourceLocation& location, const std::string& message)
    : m_location(location)
    , m_message(message)
{
}

const SourceLocation& Parser::Error::location() const
{
    return m_location;
}

const std::string& Parser::Error::message() const
{
    return m_message;
}

void Parser::unexpectedToken(const Token& t)
{
    std::stringstream message;
    message << "Unexpected token: " << t.lexeme();
    m_errors.emplace_back(Error {
        t.location,
        message.str(),
    });
}

void Parser::unexpectedToken(const Token& t, Token::Type)
{
    // TODO: better message
    unexpectedToken(t);
}

void Parser::reportErrors(std::ostream& out)
{
    for (const auto& error : m_errors) {
        out << error.location() << ": " << error.message() << std::endl;
    }
}
