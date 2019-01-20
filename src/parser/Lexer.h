#pragma once

#include "Token.h"

class Lexer {
public:
  Lexer(SourceFile);

  bool eof();
  Token next();
  const Token& peek() const;

private:
  void checkKeyword();

  void nextChar();
  void nextToken();
  void skipWhitespaces();
  void resetPosition();
  Token::Type nextTokenType();

  char m_nextChar;
  Token m_token;
  SourceFile m_sourceFile;
  SourcePosition m_lastPosition { 1, 1, 0 };
  SourcePosition m_position { 1, 1, 0 };
};
