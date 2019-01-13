#include "Parser.h"

#include "Assert.h"
#include "Lexer.h"

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
        return std::make_unique<StatementDeclaration>(parseStatement(t));
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
    ASSERT(t.type == Token::FUNCTION, "Function declaration must begin with `function`");

    auto fn = std::make_unique<FunctionDeclaration>(t);
    fn->name = parseIdentifier(m_lexer.next());

    ASSERT(m_lexer.next().type == Token::L_PAREN, "Expected opening parenthesis for formal parameters list");
    while (m_lexer.peek().type != Token::R_PAREN) {
        fn->parameters.emplace_back(parseIdentifier(m_lexer.next()));
        if (m_lexer.peek().type == Token::COMMA) m_lexer.next();
        else
            break;
    }
    ASSERT(m_lexer.next().type == Token::R_PAREN, "Expected closing parenthesis after formal parameters list");

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
        auto expr = std::make_unique<ExpressionStatement>(parseExpression(t));
        //ASSERT(m_lexer.peek().type == Token::SEMICOLON, "Expected semicolon after lexical expression");
        m_lexer.next();
        return expr;
    }
}

std::unique_ptr<BlockStatement> Parser::parseBlockStatement(const Token& t)
{
    ASSERT(t.type == Token::L_BRACE, "Block must begin with `{` (R_BRACE)");

    auto block = std::make_unique<BlockStatement>(t);

    while (m_lexer.peek().type != Token::R_BRACE) {
        block->declarations.emplace_back(parseDeclaration(m_lexer.next()));
    }

    ASSERT(m_lexer.next().type == Token::R_BRACE, "Block must end with `}` (R_BRACE)");

    return block;
}

std::unique_ptr<IfStatement> Parser::parseIfStatement(const Token& t)
{
    ASSERT(t.type == Token::IF, "If statement must start from the `if` keyword");

    auto ifStmt = std::make_unique<IfStatement>(t);

    ASSERT(m_lexer.next().type == Token::L_PAREN, "Expected opening parenthesis after `if`");
    ifStmt->condition = parseExpression(m_lexer.next());
    ASSERT(m_lexer.next().type == Token::R_PAREN, "Expected closing parenthesis after if condition");

    ifStmt->consequent = parseStatement(m_lexer.next());

    if (m_lexer.peek().type == Token::ELSE) {
        m_lexer.next();
        ifStmt->alternate = parseStatement(m_lexer.next());
    }

    return ifStmt;
}

std::unique_ptr<ForStatement> Parser::parseForStatement(const Token& t)
{
    ASSERT(t.type == Token::FOR, "ForStatement should begin with Token::FOR");
    return nullptr;
}

std::unique_ptr<WhileStatement> Parser::parseWhileStatement(const Token& t)
{
    return nullptr;
}

std::unique_ptr<ReturnStatement> Parser::parseReturnStatement(const Token& t)
{
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
    while (!stop)
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
    ASSERT(m_lexer.next().type == Token::L_PAREN, "CallExpression must begin with L_PAREN");
    auto call = std::make_unique<CallExpression>(std::move(callee));
    while (m_lexer.peek().type != Token::R_PAREN) {
        call->arguments.emplace_back(parseExpression(m_lexer.next()));
        if (m_lexer.peek().type == Token::COMMA)
            m_lexer.next();
        else
            break;
    }
    ASSERT(m_lexer.next().type == Token::R_PAREN, "Expected R_PAREN after function call arguments");

    return call;
}

std::unique_ptr<SubscriptExpression> Parser::parseSubscriptExpression(std::unique_ptr<Expression> target)
{
    ASSERT(m_lexer.next().type == Token::L_SQUARE, "SubscriptExpression must begin with L_SQUARE");
    auto subscript = std::make_unique<SubscriptExpression>(std::move(target));
    subscript->index = parseExpression(m_lexer.next());
    ASSERT(m_lexer.next().type == Token::R_SQUARE, "Expected R_SQUARE after subscript index");

    return subscript;
}

std::unique_ptr<MemberExpression> Parser::parseMemberExpression(std::unique_ptr<Expression> object)
{
    ASSERT(m_lexer.next().type == Token::DOT, "MemberExpression must begin with DOT");
    auto expr = std::make_unique<MemberExpression>(std::move(object));
    expr->property = parseIdentifier(m_lexer.next());
    return expr;
}

std::unique_ptr<MethodCallExpression> Parser::parseMethodCallExpression(std::unique_ptr<Expression> object)
{
    ASSERT(m_lexer.next().type == Token::ARROW, "MethodCallExpression must begin with ARROW");
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
    case Token::THIS:
        return parseThisExpression(t);
    case Token::IDENTIFIER:
        return parseIdentifier(t);
    default:
        return std::make_unique<LiteralExpression>(parseLiteral(t));
    }
}

std::unique_ptr<ArrayLiteralExpression> Parser::parseArrayLiteralExpression(const Token& t)
{
    ASSERT(t.type == Token::L_SQUARE, "ArrayLiteralExpression must begin with `[`");
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
    ASSERT(tok.type == Token::R_SQUARE, "Unterminated array literal");
    return array;
}

std::unique_ptr<ObjectLiteralExpression> Parser::parseObjectLiteralExpression(const Token& t)
{
    ASSERT(t.type == Token::L_BRACE, "ObjectLiteralExpression must begin with `{`");
    auto record = std::make_unique<ObjectLiteralExpression>(t);
    Token tok = m_lexer.next();
    while (tok.type != Token::R_BRACE) {
        auto name = parseIdentifier(tok);
        ASSERT(m_lexer.next().type == Token::EQUAL, "Expected = (EQUAL sign) between record field name and value");
        auto value = parseExpression(m_lexer.next());
        record->fields[std::move(name)] = std::move(value);
        tok = m_lexer.next();
        if (tok.type == Token::COMMA)
            tok = m_lexer.next();
        else
            break;
    }
    ASSERT(tok.type == Token::R_BRACE, "Unterminated object literal");
    return record;
}

std::unique_ptr<ThisExpression> Parser::parseThisExpression(const Token& t)
{
    ASSERT(t.type == Token::THIS, "ThisExpression must begin with `this`");
    return std::make_unique<ThisExpression>(t);
}

std::unique_ptr<Identifier> Parser::parseIdentifier(const Token& t)
{
    ASSERT(t.type == Token::IDENTIFIER, "Expected an identifier");
    return std::make_unique<Identifier>(t);
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
        ASSERT(false, "Unexpected token: `%d`", t.type);
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
