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

class Parser::EndsWith {
public:
    EndsWith(Parser& parser, Token::Type tokenType)
        : m_parser(parser)
        , m_lastEndsWith(parser.m_endsWithToken)
    {
        parser.m_endsWithToken = tokenType;
    }

    ~EndsWith()
    {
        m_parser.m_endsWithToken = m_lastEndsWith;
    }

private:
    Parser& m_parser;
    Token::Type m_lastEndsWith;
};

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
        return wrap<StatementDeclaration>(parseStatement(t, IsTopLevel::Yes));
    };
}

std::unique_ptr<LexicalDeclaration> Parser::parseLexicalDeclaration(const Token& t)
{
    auto decl = std::make_unique<LexicalDeclaration>(t);
    decl->name = parseIdentifier(m_lexer.next());

    if (m_lexer.peek().type == Token::COLON) {
        CONSUME(Token::COLON);
        decl->type = parseExpression(m_lexer.next());
    }

    CONSUME(Token::EQUAL);
    decl->initializer = parseExpression(m_lexer.next());

    //ASSERT(m_lexer.peek().type == Token::SEMICOLON, "Expected semicolon after lexical declaration");
    //m_lexer.next();

    return decl;
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration(const Token& t)
{
    CHECK(t, Token::FUNCTION);

    auto fn = std::make_unique<FunctionDeclaration>(t);

    if (m_lexer.peek().type == Token::IDENTIFIER)
        fn->name = parseIdentifier(m_lexer.next());
    else
        fn->name = parseOperator(m_lexer.getOperator(Token::UNKNOWN));

    CONSUME(Token::L_PAREN);
    while (m_lexer.peek().type != Token::R_PAREN) {
        fn->parameters.emplace_back(parseTypedIdentifier(m_lexer.next()));
        if (m_lexer.peek().type == Token::COMMA) m_lexer.next();
        else
            break;
    }
    CONSUME(Token::R_PAREN);
    CONSUME(Token::ARROW);
    fn->returnType = parseExpression(m_lexer.next());
    fn->body = parseBlockStatement(m_lexer.next());

    return fn;
}

std::unique_ptr<Statement> Parser::parseStatement(const Token& t, IsTopLevel isTopLevel)
{
    switch (t.type) {
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
    case Token::L_BRACE:
        if (isTopLevel == IsTopLevel::No)
            return parseBlockStatement(t);
        // fallthrough;
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
        return parseFunctionTypeExpression(std::move(expr));
    case Token::PIPE:
        return parseUnionTypeExpression(std::move(expr));
    default:
        if (m_lexer.peekIsOperator())
            return parseBinaryExpression(std::move(expr), stop);

        *stop = true;
        return expr;
    }
}

std::unique_ptr<CallExpression> Parser::parseCallExpression(std::unique_ptr<Expression> callee)
{
    CONSUME(Token::L_PAREN);

    std::unique_ptr<CallExpression> call;
    if (auto* memberExpression = dynamic_cast<MemberExpression*>(callee.get())) {
        call = std::make_unique<CallExpression>(std::move(memberExpression->property));
        call->arguments.emplace_back(std::move(memberExpression->object));
    } else
        call = std::make_unique<CallExpression>(std::move(callee));

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

std::unique_ptr<Expression> Parser::parseSubscriptExpression(std::unique_ptr<Expression> target)
{
    auto firstToken = m_lexer.next();
    CHECK(firstToken, Token::L_SQUARE);
    auto tok = m_lexer.next();
    if (tok.type == Token::R_SQUARE) {
        auto array = std::make_unique<ArrayTypeExpression>(tok);
        array->itemType = std::move(target);
        return std::move(array);
    }
    auto subscript = std::make_unique<SubscriptExpression>(std::move(target));
    subscript->index = parseExpression(tok);
    CONSUME(Token::R_SQUARE);

    return std::move(subscript);
}

std::unique_ptr<Expression> Parser::parseMemberExpression(std::unique_ptr<Expression> object)
{
    CONSUME(Token::DOT);

    if (m_lexer.peek().type == Token::L_PAREN) {
        auto parenthesizedExpression = std::make_unique<ParenthesizedExpression>(object->location);
        parenthesizedExpression->expression = std::move(object);
        return parseCallExpression(std::move(parenthesizedExpression));
    }

    auto expr = std::make_unique<MemberExpression>(std::move(object));
    expr->property = parseIdentifier(m_lexer.next());
    return std::move(expr);
}

std::unique_ptr<FunctionTypeExpression> Parser::parseFunctionTypeExpression(std::unique_ptr<Expression> parameters)
{
    CONSUME(Token::ARROW);

    auto type = std::make_unique<FunctionTypeExpression>(parameters->location);
    if (auto* tuple = dynamic_cast<TupleExpression*>(parameters.get()))
        type->parameters = std::move(tuple->items);
    else
        type->parameters.emplace_back(std::move(parameters));
    type->returnType = parseExpression(m_lexer.next());
    return type;
}

std::unique_ptr<UnionTypeExpression> Parser::parseUnionTypeExpression(std::unique_ptr<Expression> lhs)
{
    CONSUME(Token::PIPE);

    auto type = std::make_unique<UnionTypeExpression>(lhs->location);
    type->lhs = std::move(lhs);
    type->rhs = parseExpression(m_lexer.next());
    return type;
}

std::unique_ptr<Expression> Parser::parseBinaryExpression(std::unique_ptr<Expression> lhs, bool* stop)
{
    auto op = m_lexer.getOperator(m_endsWithToken);
    if (op.type != Token::OPERATOR) {
        *stop = true;
        return lhs;
    }

    auto expr = std::make_unique<CallExpression>(parseOperator(op));
    expr->arguments.emplace_back(std::move(lhs));
    expr->arguments.emplace_back(parseExpression(m_lexer.next()));
    return expr;
}
std::unique_ptr<Expression> Parser::parsePrimaryExpression(const Token &t)
{
    switch (t.type) {
    case Token::L_SQUARE: {
        EndsWith endsWith(*this, Token::R_SQUARE);
        return parseArrayLiteralExpression(t);
    }
    case Token::L_BRACE: {
        EndsWith endsWith(*this, Token::R_BRACE);
        return parseObjectLiteralExpression(t);
    }
    case Token::L_PAREN: {
        EndsWith endsWith(*this, Token::R_PAREN);
        return parseParenthesizedExpressionOrTuple(t);
    }
    case Token::L_ANGLE: {
        EndsWith endsWith(*this, Token::R_ANGLE);
        return parseTupleTypeExpression(t);
    }
    //case Token::FUNCTION:
        //return parseFunctionExpression(t);
    case Token::IDENTIFIER:
        return parseIdentifier(t);
    case Token::TYPE:
        return parseTypeType(t);
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

std::unique_ptr<Expression> Parser::parseObjectLiteralExpression(const Token& t)
{
    CHECK(t, Token::L_BRACE);

    Token tok = m_lexer.next();
    if (tok.type == Token::R_BRACE)
        return std::make_unique<ObjectLiteralExpression>(t);
    if (tok.type == Token::COLON) {
        CONSUME(Token::R_BRACE);
        return std::make_unique<ObjectTypeExpression>(t);
    }

    auto name = parseIdentifier(tok);
    auto parse = [&](Token::Type separator, auto&& object) -> std::unique_ptr<Expression> {
        CHECK(tok, separator);
        for (;;) {
            auto value = parseExpression(m_lexer.next());
            object->fields[std::move(name)] = std::move(value);
            tok = m_lexer.next();
            if (tok.type != Token::COMMA)
                break;
            tok = m_lexer.next();
            if (tok.type == Token::R_BRACE)
                break;
            name = parseIdentifier(tok);
            CONSUME(separator);
        }
        CHECK(tok, Token::R_BRACE);
        return std::move(object);
    };

    tok = m_lexer.next();
    switch (tok.type) {
    case Token::COLON:
        return parse(Token::COLON, std::make_unique<ObjectTypeExpression>(t));
    case Token::EQUAL:
        return parse(Token::EQUAL, std::make_unique<ObjectLiteralExpression>(t));
    default:
        unexpectedToken(tok);
        return nullptr;
    }
}

std::unique_ptr<Expression> Parser::parseParenthesizedExpressionOrTuple(const Token& t)
{
    CHECK(t, Token::L_PAREN);

    if (m_lexer.peekIsOperator()) {
        auto op = parseOperator(m_lexer.getOperator(Token::R_PAREN));
        CONSUME(Token::R_PAREN);
        return std::move(op);
    }

    auto expr = parseExpression(m_lexer.next());
    auto tok = m_lexer.next();
    switch (tok.type) {
    case Token::R_PAREN: {
        auto parenthesizedExpression = std::make_unique<ParenthesizedExpression>(t);
        parenthesizedExpression->expression = std::move(expr);
        return std::move(parenthesizedExpression);
    }
    case Token::COMMA: {
        auto tuple = std::make_unique<TupleExpression>(t);
        tuple->items.emplace_back(std::move(expr));
        for (;;) {
            tuple->items.emplace_back(parseExpression(m_lexer.next()));
            if (m_lexer.peek().type != Token::COMMA)
                break;
            CONSUME(Token::COMMA);
        }
        CONSUME(Token::R_PAREN);
        return std::move(tuple);
    }
    default:
        unexpectedToken(tok);
    }
    return nullptr;
}

std::unique_ptr<TupleTypeExpression> Parser::parseTupleTypeExpression(const Token& t)
{
    CHECK(t, Token::L_ANGLE);
    auto tupleType = std::make_unique<TupleTypeExpression>(t);
    while (m_lexer.peek().type != Token::R_ANGLE) {
        tupleType->items.push_back(parseExpression(m_lexer.next()));
        if (m_lexer.peek().type != Token::COMMA)
            break;
        m_lexer.next();
    }
    CONSUME(Token::R_ANGLE);
    if (tupleType->items.size() < 2)
        parseError(t, "Tuple type should have at least two members");
    return tupleType;
}

std::unique_ptr<Identifier> Parser::parseIdentifier(const Token& t)
{
    CHECK(t, Token::IDENTIFIER);
    return std::make_unique<Identifier>(t);
}

std::unique_ptr<Identifier> Parser::parseOperator(const Token& t)
{
    CHECK(t, Token::OPERATOR);
    return std::make_unique<Identifier>(t, true);
}

std::unique_ptr<TypedIdentifier> Parser::parseTypedIdentifier(const Token& t)
{
    auto typedIdentifier = std::make_unique<TypedIdentifier>(t);
    if (t.type == Token::MOD) {
        typedIdentifier->inferred = true;
        typedIdentifier->name = parseIdentifier(m_lexer.next());
    } else {
        typedIdentifier->inferred = false;
        typedIdentifier->name = parseIdentifier(t);
    }
    CONSUME(Token::COLON);
    typedIdentifier->type = parseExpression(m_lexer.next());
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
    uint32_t start = t.location.start.offset;
    str->value = std::string(t.location.file.source + start + 1, t.location.end.offset - start - 2);
    return str;
}

std::unique_ptr<BooleanLiteral> Parser::parseBooleanLiteral(const Token& t, bool value)
{
    auto boolean = std::make_unique<BooleanLiteral>(t);
    boolean->value = value;
    return boolean;
}

std::unique_ptr<TypeTypeExpression> Parser::parseTypeType(const Token& t)
{
    CHECK(t, Token::TYPE);
    return std::make_unique<TypeTypeExpression>(t);
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

void Parser::parseError(const Token& t, const std::string& message)
{
    m_errors.emplace_back(Error {
        t.location,
        message,
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
