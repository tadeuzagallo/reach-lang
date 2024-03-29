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

    // TODO: if we have a type declaration there's we can have a checked expression here
    if (m_lexer.peek().type == Token::COLON) {
        CONSUME(Token::COLON);
        decl->type = parseInferredExpression(m_lexer.next());
    }

    CONSUME(Token::EQUAL);
    decl->initializer = parseInferredExpression(m_lexer.next());

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
    fn->returnType = parseInferredExpression(m_lexer.next());
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
    case Token::MATCH:
        return parseMatchStatement(t);
    case Token::L_BRACE:
        if (isTopLevel == IsTopLevel::No)
            return parseBlockStatement(t);
        // fallthrough;
    default:
        auto expr = wrap<ExpressionStatement>(parseInferredExpression(t));
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
    EndsWith endsWith(*this, Token::R_PAREN);
    ifStmt->condition = parseCheckedExpression(m_lexer.next());
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
    ret->expression = parseInferredExpression(next);
    return ret;
}

std::unique_ptr<CheckedExpression> Parser::parseCheckedExpression(const Token& t)
{
    // TODO: parse lambda
    return parseInferredExpression(t);
}

std::unique_ptr<InferredExpression> Parser::parseInferredExpression(const Token& t)
{
    auto expr = parsePrimaryExpression(t);
    bool stop = false;
    while (expr && !stop)
        expr = parseSuffixExpression(std::move(expr), &stop);
    return expr;
}

std::unique_ptr<InferredExpression> Parser::parseSuffixExpression(std::unique_ptr<InferredExpression> expr, bool* stop)
{
    switch (m_lexer.peek().type) {
    case Token::L_PAREN:
        return parseCallExpression(std::move(expr));
    case Token::L_SQUARE:
        return parseSubscriptExpressionOrArrayType(std::move(expr));
    case Token::DOT:
        return parseMemberExpressionOrUFCSCall(std::move(expr));
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

std::unique_ptr<CallExpression> Parser::parseCallExpression(std::unique_ptr<InferredExpression> callee)
{
    CONSUME(Token::L_PAREN);

    std::unique_ptr<CallExpression> call;
    if (auto* memberExpression = dynamic_cast<MemberExpression*>(callee.get())) {
        call = std::make_unique<CallExpression>(std::move(memberExpression->property));
        call->arguments.emplace_back(std::move(memberExpression->object));
    } else
        call = std::make_unique<CallExpression>(std::move(callee));

    while (m_lexer.peek().type != Token::R_PAREN) {
        call->arguments.emplace_back(parseCheckedExpression(m_lexer.next()));
        if (m_lexer.peek().type == Token::COMMA)
            m_lexer.next();
        else
            break;
    }
    CONSUME(Token::R_PAREN);

    return call;
}

std::unique_ptr<InferredExpression> Parser::parseSubscriptExpressionOrArrayType(std::unique_ptr<InferredExpression> target)
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
    subscript->index = parseCheckedExpression(tok);
    CONSUME(Token::R_SQUARE);

    return std::move(subscript);
}

std::unique_ptr<InferredExpression> Parser::parseMemberExpressionOrUFCSCall(std::unique_ptr<InferredExpression> object)
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

std::unique_ptr<FunctionTypeExpression> Parser::parseFunctionTypeExpression(std::unique_ptr<InferredExpression> parameters)
{
    CONSUME(Token::ARROW);

    auto type = std::make_unique<FunctionTypeExpression>(parameters->location);
    if (auto* tuple = dynamic_cast<TupleExpression*>(parameters.get()))
        type->parameters = std::move(tuple->items);
    else
        type->parameters.emplace_back(std::move(parameters));
    type->returnType = parseInferredExpression(m_lexer.next());
    return type;
}

std::unique_ptr<UnionTypeExpression> Parser::parseUnionTypeExpression(std::unique_ptr<InferredExpression> lhs)
{
    CONSUME(Token::PIPE);

    auto type = std::make_unique<UnionTypeExpression>(lhs->location);
    type->lhs = std::move(lhs);
    type->rhs = parseInferredExpression(m_lexer.next());
    return type;
}

std::unique_ptr<InferredExpression> Parser::parseBinaryExpression(std::unique_ptr<InferredExpression> lhs, bool* stop)
{
    auto op = m_lexer.getOperator(m_endsWithToken);
    if (op.type != Token::OPERATOR) {
        *stop = true;
        return lhs;
    }

    auto expr = std::make_unique<CallExpression>(parseOperator(op));
    expr->arguments.emplace_back(std::move(lhs));
    expr->arguments.emplace_back(parseCheckedExpression(m_lexer.next()));
    return expr;
}
std::unique_ptr<InferredExpression> Parser::parsePrimaryExpression(const Token &t)
{
    switch (t.type) {
    case Token::L_SQUARE: {
        EndsWith endsWith(*this, Token::R_SQUARE);
        return parseArrayLiteralExpression(t);
    }
    case Token::L_BRACE: {
        EndsWith endsWith(*this, Token::R_BRACE);
        return parseObjectLiteralExpressionOrObjectType(t);
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
    case Token::HASH:
        return parseLazyExpression(t);
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
        array->items.push_back(parseInferredExpression(tok));
        tok = m_lexer.next();
        if (tok.type == Token::COMMA)
            tok = m_lexer.next();
        else
            break;
    }
    CHECK(tok, Token::R_SQUARE);
    return array;
}

std::unique_ptr<InferredExpression> Parser::parseObjectLiteralExpressionOrObjectType(const Token& t)
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
    auto parse = [&](Token::Type separator, auto&& object) -> std::unique_ptr<InferredExpression> {
        CHECK(tok, separator);
        for (;;) {
            auto value = parseInferredExpression(m_lexer.next());
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

std::unique_ptr<InferredExpression> Parser::parseParenthesizedExpressionOrTuple(const Token& t)
{
    CHECK(t, Token::L_PAREN);

    if (m_lexer.peekIsOperator()) {
        auto op = parseOperator(m_lexer.getOperator(Token::R_PAREN));
        CONSUME(Token::R_PAREN);
        return std::move(op);
    }

    auto expr = parseInferredExpression(m_lexer.next());
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
            tuple->items.emplace_back(parseInferredExpression(m_lexer.next()));
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

std::unique_ptr<LazyExpression> Parser::parseLazyExpression(const Token& t)
{
    CHECK(t, Token::HASH);
    auto lazyExpression = std::make_unique<LazyExpression>(t);
    lazyExpression->expression = parseInferredExpression(m_lexer.next());
    return lazyExpression;
}

std::unique_ptr<TupleTypeExpression> Parser::parseTupleTypeExpression(const Token& t)
{
    CHECK(t, Token::L_ANGLE);
    auto tupleType = std::make_unique<TupleTypeExpression>(t);
    while (m_lexer.peek().type != Token::R_ANGLE) {
        tupleType->items.push_back(parseInferredExpression(m_lexer.next()));
        if (m_lexer.peek().type != Token::COMMA)
            break;
        m_lexer.next();
    }
    CONSUME(Token::R_ANGLE);
    if (tupleType->items.size() < 2)
        parseError(t, "Tuple type should have at least two members");
    return tupleType;
}

std::unique_ptr<MatchStatement> Parser::parseMatchStatement(const Token& t)
{
    auto match = std::make_unique<MatchStatement>(t);

    CHECK(t, Token::MATCH);
    CONSUME(Token::L_PAREN);
    {
        EndsWith endsWith(*this, Token::R_PAREN);
        match->scrutinee = parseInferredExpression(m_lexer.next());
    }
    CONSUME(Token::R_PAREN);

    CONSUME(Token::L_BRACE);
    {
        EndsWith endsWith(*this, Token::R_BRACE);
        for (;;) {
            switch (m_lexer.peek().type) {
            case Token::DEFAULT:
                if (match->defaultCase)
                    parseError(m_lexer.peek(), "Found multiple `default` cases in match statement, but it should only have one");

                CONSUME(Token::DEFAULT);
                CONSUME(Token::COLON);
                match->defaultCase = parseStatement(m_lexer.next());
                break;
            case Token::CASE:
                match->cases.emplace_back(parseMatchCase(m_lexer.next()));
                break;
            case Token::R_BRACE:
                goto end;
            default:
                unexpectedToken(m_lexer.next());
                goto end;
            }
        }
    }
end:
    CONSUME(Token::R_BRACE);

    if (!match->cases.size() && !match->defaultCase)
        parseError(t, "Invalid `match` statement with no cases");

    return match;
}

std::unique_ptr<MatchCase> Parser::parseMatchCase(const Token& t)
{
    auto kase = std::make_unique<MatchCase>(t);
    CHECK(t, Token::CASE);
    kase->pattern = parsePattern(m_lexer.next());
    CONSUME(Token::COLON);
    kase->statement = parseStatement(m_lexer.next());
    return kase;
}

std::unique_ptr<Pattern> Parser::parsePattern(const Token& t)
{
    switch (t.type) {
    case Token::IDENTIFIER:
        return parseIdentifierPattern(t);
    case Token::UNDERSCORE:
        return parseUnderscorePattern(t);
    case Token::L_BRACE: {
        EndsWith endsWith(*this, Token::R_BRACE);
        return parseObjectPattern(t);
    }
    default:
        return parseLiteralPattern(t);
    }
}

std::unique_ptr<IdentifierPattern> Parser::parseIdentifierPattern(const Token& t)
{
    auto pattern = std::make_unique<IdentifierPattern>(t);
    pattern->name = parseIdentifier(t);
    return pattern;
}

std::unique_ptr<ObjectPattern> Parser::parseObjectPattern(const Token& t)
{
    auto pattern = std::make_unique<ObjectPattern>(t);
    CHECK(t, Token::L_BRACE);
    while (m_lexer.peek().type != Token::R_BRACE) {
        auto name = parseIdentifier(m_lexer.next());
        CONSUME(Token::EQUAL);
        auto value = parsePattern(m_lexer.next());
        pattern->entries[std::move(name)] = std::move(value);
        if (m_lexer.peek().type != Token::COMMA)
            break;
        CONSUME(Token::COMMA);
    }
    CONSUME(Token::R_BRACE);
    return pattern;
}

std::unique_ptr<UnderscorePattern> Parser::parseUnderscorePattern(const Token& t)
{
    CHECK(t, Token::UNDERSCORE);
    return std::make_unique<UnderscorePattern>(t);
}

std::unique_ptr<LiteralPattern> Parser::parseLiteralPattern(const Token& t)
{
    auto pattern = std::make_unique<LiteralPattern>(t);
    pattern->literal = parseLiteral(t);
    return pattern;
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
    if (m_lexer.peek().type == Token::LESS_COLON) {
        CONSUME(Token::LESS_COLON);
        typedIdentifier->isSubtype = true;
    } else
        CONSUME(Token::COLON);
    typedIdentifier->type = parseInferredExpression(m_lexer.next());
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
