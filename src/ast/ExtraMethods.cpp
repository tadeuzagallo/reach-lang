#include "declarations.h"
#include "expressions.h"
#include "literals.h"
#include "program.h"
#include "statements.h"

void Program::dump(std::ostream& out)
{
  dump(out, 0);
}

Expression::Expression(const Token& t)
    : Node(t)
{
}

CheckedExpression::CheckedExpression(const Token& t)
    : Expression(t)
{
}

StatementDeclaration::StatementDeclaration(std::unique_ptr<Statement> stmt)
  : Declaration(stmt->location)
  , statement(std::move(stmt))
{ }

Identifier::Identifier(const Token& t, bool isOperator)
  : InferredExpression(t)
  , name(t.lexeme())
  , isOperator(isOperator)
{ }

bool Identifier::operator<(const Identifier& other) const
{
    return name < other.name;
}

std::ostream& operator<<(std::ostream& out, const Identifier& ident)
{
    out << ident.name;
    return out;
}

LiteralExpression::LiteralExpression(std::unique_ptr<Literal> lit)
  : InferredExpression(lit->location)
  , literal(std::move(lit))
{ }

CallExpression::CallExpression(std::unique_ptr<InferredExpression> expr)
  : InferredExpression(expr->location)
  , callee(std::move(expr))
{ }

MemberExpression::MemberExpression(std::unique_ptr<InferredExpression> expr)
  : InferredExpression(expr->location)
  , object(std::move(expr))
{ }

SubscriptExpression::SubscriptExpression(std::unique_ptr<InferredExpression> expr)
  : InferredExpression(expr->location)
  , target(std::move(expr))
{ }

ExpressionStatement::ExpressionStatement(std::unique_ptr<InferredExpression> expr)
  : Statement(expr->location)
  , expression(std::move(expr))
{ }
