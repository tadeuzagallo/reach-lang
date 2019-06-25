#include "Lexer.h"

#include "Assert.h"
#include <ctype.h>
#include <string.h>

Lexer::Lexer(SourceFile sourceFile)
    : m_sourceFile(sourceFile)
{
    nextChar();
    nextToken();
}

bool Lexer::eof()
{
    return m_position.offset == m_sourceFile.length;
}

const Token& Lexer::peek() const
{
    return m_token;
}

Token Lexer::next()
{
    Token t = m_token;
    nextToken();
    return t;
}

void Lexer::nextToken()
{
    skipWhitespaces();
    resetPosition();
    m_token.type = nextTokenType();
    m_token.location.end = m_lastPosition;

    if (m_token.type == Token::IDENTIFIER)
        checkKeyword();
}

void Lexer::skipWhitespaces()
{
    while (isspace(m_nextChar))
        nextChar();
}

void Lexer::resetPosition()
{
    m_token.location.file = m_sourceFile;
    m_token.location.start = m_lastPosition;
}

void Lexer::nextChar()
{
    if (eof()) {
        m_nextChar = '\0';
        return;
    }

    m_lastPosition = m_position;
    m_nextChar = m_sourceFile.source[m_position.offset++];
    if (m_nextChar == '\n') {
        m_position.line++;
        m_position.column = 1;
    } else {
        m_position.column++;
    }
}

Token::Type Lexer::nextTokenType()
{
#define SIMPLE_CASE(__char, __type) \
    case __char: \
        nextChar(); \
        return Token::__type

    switch (m_nextChar) {
        SIMPLE_CASE('\0', END_OF_FILE);
        SIMPLE_CASE('.', DOT);
        SIMPLE_CASE(',', COMMA);
        SIMPLE_CASE(':', COLON);
        SIMPLE_CASE(';', SEMICOLON);

        SIMPLE_CASE('+', PLUS);
        SIMPLE_CASE('%', MOD);

        SIMPLE_CASE('(', L_PAREN);
        SIMPLE_CASE(')', R_PAREN);
        SIMPLE_CASE('{', L_BRACE);
        SIMPLE_CASE('}', R_BRACE);
        SIMPLE_CASE('[', L_SQUARE);
        SIMPLE_CASE(']', R_SQUARE);
        SIMPLE_CASE('<', L_ANGLE);
        SIMPLE_CASE('>', R_ANGLE);

        default:
        if (m_nextChar == '/') {
            nextChar();
            switch (m_nextChar) {
            case '/':
                do nextChar();
                while (m_nextChar != '\n');
                skipWhitespaces();
                resetPosition();
                return nextTokenType();
            // TODO: multiline comments
            default:
                return Token::DIVIDE;
            }
        }

        if (isdigit(m_nextChar)) {
            do nextChar();
            while (isdigit(m_nextChar));
            return Token::NUMBER;
        }

        if (isalpha(m_nextChar) || m_nextChar == '_' || m_nextChar == '$') {
            do nextChar();
            while (isalpha(m_nextChar) || m_nextChar == '_' || m_nextChar == '$');
            return Token::IDENTIFIER;
        }

        if (m_nextChar == '"' || m_nextChar == '\'') {
            auto init = m_nextChar;
            do {
                if (m_nextChar == '\\')
                    nextChar();
                nextChar();
            } while (m_nextChar != init && m_nextChar != '\0');
            ASSERT(m_nextChar == init, "Unterminated string");
            nextChar();
            return Token::STRING;
        }

#define TOKEN2(__chars, __type2, __type1) \
        do { \
            if (m_nextChar == __chars[0]) { \
                nextChar(); \
                if (m_nextChar == __chars[1]) { \
                    nextChar(); \
                    return Token::__type2; \
                } else { \
                    return Token::__type1; \
                } \
            } \
        } while (false)

        TOKEN2("==", EQUAL_EQUAL, EQUAL);
        TOKEN2("!=", NOT_EQUAL, BANG);
        TOKEN2("**", POWER, TIMES);
        TOKEN2("->", ARROW, MINUS);

#undef TOKEN2

#define TOKEN2(__chars, __type) \
        do { \
            if (m_nextChar == __chars[0]) { \
                nextChar(); \
                ASSERT(m_nextChar == __chars[1], "Expected `%c` after `%c`, but found `%c`\n", __chars[1], __chars[0], m_nextChar); \
                return Token::__type; \
            } \
        } while (false)

        TOKEN2("&&", AND);
        TOKEN2("||", OR);

#undef TOKEN2

        ASSERT(false, "Unknown token: `%c` (%d)\n", m_nextChar, m_nextChar);
        return Token::END_OF_FILE;
    };

#undef SIMPLE_CASE
}

void Lexer::checkKeyword()
{
    ASSERT(m_token.type == Token::IDENTIFIER, "Only identifiers can be promoted to keywords\n");
    size_t length = m_token.location.end.offset - m_token.location.start.offset;
    const char* lexeme = m_sourceFile.source + m_token.location.start.offset;

#define KEYWORD(__keyword, __type) \
    if (length == strlen(#__keyword) && !strncmp(lexeme, #__keyword, length)) { \
        m_token.type = Token::__type; \
        return; \
    }

    KEYWORD(if, IF)
    KEYWORD(else, ELSE)
    KEYWORD(let, LET)
    KEYWORD(const, CONST)
    KEYWORD(for, FOR)
    KEYWORD(while, WHILE)
    KEYWORD(function, FUNCTION)
    KEYWORD(return, RETURN)
    KEYWORD(true, TRUE)
    KEYWORD(false, FALSE)
    KEYWORD(Type, TYPE)

#undef KEYWORD
}
