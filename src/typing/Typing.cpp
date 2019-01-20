#include "AST.h"
#include "Type.h"
#include "TypeChecker.h"

// Declarations

const Type& Declaration::infer(TypeChecker& tc)
{
    check(tc, tc.unitType());
    return tc.unitType();
}

void LexicalDeclaration::check(TypeChecker&, const Type&)
{
    // TODO
}

void FunctionDeclaration::check(TypeChecker& tc, const Type& result)
{
    Types params;
    for (uint32_t i = 0; i < parameters.size(); i++)
        params.emplace_back(parameters[i]->type->normalize(tc));
    const Type& retType = returnType->normalize(tc);
    body->check(tc, retType);
    tc.insert(name->name, Type::function(params, retType));
    tc.checkEquals(location, result, tc.unitType());
}

const Type& StatementDeclaration::infer(TypeChecker& tc)
{
    return statement->infer(tc);
}

void StatementDeclaration::check(TypeChecker& tc, const Type& result)
{
    statement->check(tc, result);
}


// Statements

const Type& Statement::infer(TypeChecker& tc)
{
    check(tc, tc.unitType());
    return tc.unitType();
}

void EmptyStatement::check(TypeChecker&, const Type&)
{
    // nothing to do here
}

void BlockStatement::check(TypeChecker& tc, const Type& result)
{
    for (const auto& decl : declarations) {
        if (decl == declarations.back())
            decl->check(tc, result);
        else
            decl->check(tc, tc.unitType());
    }
}

void ReturnStatement::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& IfStatement::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitType();
}

void IfStatement::check(TypeChecker&, const Type&)
{
    // TODO
}

void BreakStatement::check(TypeChecker&, const Type&)
{
    // TODO
}

void ContinueStatement::check(TypeChecker&, const Type&)
{
    // TODO
}

void WhileStatement::check(TypeChecker&, const Type&)
{
    // TODO
}

void ForStatement::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& ExpressionStatement::infer(TypeChecker& tc)
{
    return expression->infer(tc);
}

void ExpressionStatement::check(TypeChecker& tc, const Type& result)
{
    expression->check(tc, result);
}


// Expressions

const Type& Identifier::infer(TypeChecker& tc)
{
    return tc.lookup(name);
}

void Identifier::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& BinaryExpression::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitType();
}

void BinaryExpression::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& ParenthesizedExpression::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitType();
}

void ParenthesizedExpression::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& ObjectLiteralExpression::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitType();
}

void ObjectLiteralExpression::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& ArrayLiteralExpression::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitType();
}

void ArrayLiteralExpression::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& CallExpression::infer(TypeChecker& tc)
{
    // TODO
    const Type& calleeType = callee->infer(tc);
    if (!calleeType.isFunction())
        tc.typeError(location, "Callee is not a function");
    const TypeFunction& calleeTypeFunction = calleeType.asFunction();
    if (calleeTypeFunction.paramCount() != arguments.size())
        tc.typeError(location, "Argument count mismatch");
    for (unsigned i = 0; i < arguments.size(); i++)
        arguments[i]->check(tc, calleeTypeFunction.param(i));
    return calleeTypeFunction.returnType();
}

void CallExpression::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& SubscriptExpression::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitType();
}

void SubscriptExpression::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& MemberExpression::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitType();
}

void MemberExpression::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& MethodCallExpression::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitType();
}

void MethodCallExpression::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& LiteralExpression::infer(TypeChecker& tc)
{
    return literal->infer(tc);
}

void LiteralExpression::check(TypeChecker& tc, const Type& result)
{
    tc.checkEquals(location, infer(tc), result);
}


// Literals

const Type& BooleanLiteral::infer(TypeChecker& tc)
{
    return tc.booleanType();
}

const Type& NumericLiteral::infer(TypeChecker& tc)
{
    return tc.numericType();
}

const Type& StringLiteral::infer(TypeChecker& tc)
{
    return tc.stringType();
}

// Types
const Type& ASTTypeName::normalize(TypeChecker& tc)
{
    return tc.lookup(name->name);
}
