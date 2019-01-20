#include "AST.h"
#include "Type.h"
#include "TypeChecker.h"

// Declarations

const Type& Declaration::infer(TypeChecker& tc)
{
    check(tc, tc.unitType());
    return tc.unitType();
}

void LexicalDeclaration::check(TypeChecker& tc, const Type& type)
{
    const Type& initializerType = (*initializer)->infer(tc);
    tc.insert(name->name, initializerType);
    tc.checkEquals(location, tc.unitType(), type);
}

void FunctionDeclaration::check(TypeChecker& tc, const Type& result)
{
    Types params;
    for (uint32_t i = 0; i < parameters.size(); i++)
        params.emplace_back(parameters[i]->type->normalize(tc));
    const Type& retType = returnType->normalize(tc);
    tc.insert(name->name, tc.newFunctionType(params, retType));

    {
        TypeChecker::Scope bodyScope(tc);
        for (uint32_t i = 0; i < params.size(); i++)
            tc.insert(parameters[i]->name->name, params[i]);
        body->check(tc, retType);
    }
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
            decl->infer(tc);
    }
}

void ReturnStatement::check(TypeChecker&, const Type&)
{
    // TODO
}

const Type& IfStatement::infer(TypeChecker& tc)
{
    condition->check(tc, tc.booleanType());
    const Type& result = consequent->infer(tc);
    if (!alternate)
        return tc.unitType();
    (*alternate)->check(tc, result);
    return result;
}

void IfStatement::check(TypeChecker& tc, const Type& type)
{
    condition->check(tc, tc.booleanType());
    consequent->check(tc, type);
    if (alternate)
        (*alternate)->check(tc, type);
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
    return tc.lookup(location, name);
}

void Identifier::check(TypeChecker& tc, const Type& type)
{
    // TODO: proper checking
    tc.checkEquals(location, infer(tc), type);
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
    return expression->infer(tc);
}

void ParenthesizedExpression::check(TypeChecker& tc, const Type& type)
{
    expression->check(tc, type);
}

const Type& ObjectLiteralExpression::infer(TypeChecker& tc)
{
    Fields typeFields;
    for (auto& pair : fields) {
        typeFields.emplace(pair.first->name, pair.second->infer(tc));
    }
    return tc.newRecordType(typeFields);
}

void ObjectLiteralExpression::check(TypeChecker& tc, const Type& type)
{
    if (!type.isRecord()) {
        tc.typeError(location, "Unexpected record type");
        infer(tc); // check for errors in the record anyhow
        return;
    }
}

const Type& ArrayLiteralExpression::infer(TypeChecker& tc)
{
    const Type& itemType = items.size()
        ? items[0]->infer(tc)
        : tc.unitType();

    for (uint32_t i = 1; i < items.size(); i++)
        items[i]->check(tc, itemType);

    return tc.newArrayType(itemType);
}

void ArrayLiteralExpression::check(TypeChecker& tc, const Type& type)
{
    if (!type.isArray()) {
        tc.typeError(location, "Unexpected array");
        infer(tc); // check for error in the array anyhow
        return;
    }

    const Type& itemType = type.asArray().itemType();
    for (const auto& item : items)
        item->check(tc, itemType);
}

const Type& CallExpression::infer(TypeChecker& tc)
{
    const Type& calleeType = callee->infer(tc);
    if (!calleeType.isFunction()) {
        tc.typeError(location, "Callee is not a function");
        return tc.unitType();
    }

    const TypeFunction& calleeTypeFunction = calleeType.asFunction();
    if (calleeTypeFunction.paramCount() != arguments.size())
        tc.typeError(location, "Argument count mismatch");
    else
        for (unsigned i = 0; i < arguments.size(); i++)
            arguments[i]->check(tc, calleeTypeFunction.param(i));
    return calleeTypeFunction.returnType();
}

void CallExpression::check(TypeChecker& tc, const Type& type)
{
    // TODO: proper checking
    tc.checkEquals(location, infer(tc), type);
}

const Type& SubscriptExpression::infer(TypeChecker& tc)
{
    const Type& targetType = target->infer(tc);
    if (!targetType.isArray()) {
        tc.typeError(location, "Trying to subscript non-array");
        return tc.unitType();
    }

    const TypeArray& targetArrayType = targetType.asArray();
    index->check(tc, tc.numericType());
    return targetArrayType.itemType();
}

void SubscriptExpression::check(TypeChecker& tc, const Type& itemType)
{
    target->check(tc, tc.newArrayType(itemType));
    index->check(tc, tc.numericType());
}

const Type& MemberExpression::infer(TypeChecker& tc)
{
    const Type& targetType = object->infer(tc);
    if (!targetType.isRecord()) {
        tc.typeError(location, "Trying to access field of non-record");
        return tc.unitType();
    }

    const TypeRecord& recordType = targetType.asRecord();
    auto optionalFieldType = recordType.field(property->name);
    if (!optionalFieldType) {
        tc.typeError(location, "Unknown field");
        return tc.unitType();
    }

    return *optionalFieldType;
}

void MemberExpression::check(TypeChecker& tc, const Type& type)
{
    // TODO: do we need something custom here?
    tc.checkEquals(location, infer(tc), type);
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
    return tc.lookup(location, name->name);
}
