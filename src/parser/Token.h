#pragma once

#include "SourceLocation.h"
#include <string>

class Token {
public:
  enum Type {
    // need lexeme
    IDENTIFIER,
    STRING,
    NUMBER,

    // keywords
    IF,
    ELSE,
    LET,
    CONST,
    FOR,
    WHILE,
    FUNCTION,
    RETURN,
    TRUE,
    FALSE,
    THIS,

    // simple tokens
    END_OF_FILE,
    DOT,
    COMMA,
    COLON,
    SEMICOLON,

    MOD,
    PLUS,
    MINUS,
    DIVIDE,

    L_PAREN,
    R_PAREN,
    L_BRACE,
    R_BRACE,
    L_SQUARE,
    R_SQUARE,

    // multi-char fixed-width tokens
    EQUAL,
    EQUAL_EQUAL,
    BANG,
    NOT_EQUAL,
    TIMES,
    POWER,
    AND,
    OR,
    ARROW,
  };

  std::string lexeme() const
  {
      uint32_t start = location.start.offset;
      return std::string(location.file.source + start, location.end.offset - start);
  }

  Type type;
  SourceLocation location;
};
