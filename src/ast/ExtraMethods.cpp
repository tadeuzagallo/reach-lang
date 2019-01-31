#include "declarations.h"
#include "expressions.h"
#include "literals.h"
#include "program.h"
#include "statements.h"

void Program::dump(std::ostream& out)
{
  dump(out, 0);
}

StatementDeclaration::StatementDeclaration(std::unique_ptr<Statement> stmt)
  : Declaration(stmt->location)
  , statement(std::move(stmt))
{ }

Identifier::Identifier(const Token& t)
  : Expression(t)
  , name(t.lexeme())
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
  : Expression(lit->location)
  , literal(std::move(lit))
{ }

TypeExpression::TypeExpression(std::unique_ptr<ASTType> type)
  : Expression(type->location)
  , type(std::move(type))
{ }

CallExpression::CallExpression(std::unique_ptr<Expression> expr)
  : Expression(expr->location)
  , callee(std::move(expr))
{ }

MemberExpression::MemberExpression(std::unique_ptr<Expression> expr)
  : Expression(expr->location)
  , object(std::move(expr))
{ }

SubscriptExpression::SubscriptExpression(std::unique_ptr<Expression> expr)
  : Expression(expr->location)
  , target(std::move(expr))
{ }

ExpressionStatement::ExpressionStatement(std::unique_ptr<Expression> expr)
  : Statement(expr->location)
  , expression(std::move(expr))
{ }
