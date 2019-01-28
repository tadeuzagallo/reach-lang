#include "AST.h"
#include "Binding.h"
#include "Type.h"
#include "TypeChecker.h"

// Declarations

const Binding& Declaration::infer(TypeChecker& tc)
{
    check(tc, tc.unitValue());
    return tc.unitValue();
}

void LexicalDeclaration::check(TypeChecker& tc, const Binding& type)
{
    const Binding& initializerBinding = (*initializer)->infer(tc);
    tc.insert(name->name, initializerBinding);
    tc.unify(location, tc.unitValue(), type);
}

void FunctionDeclaration::check(TypeChecker& tc, const Binding& result)
{
    const Binding* fnType;
    Types params;
    std::function<void(uint32_t)> check = [&](uint32_t i) {
        TypeChecker::Scope scope(tc);
        if (i < parameters.size()) {
            const Binding& binding = parameters[i]->normalize(tc);
            params.emplace_back(binding);
            tc.insert(parameters[i]->name->name, binding);
            check(i + 1);
            return;
        }

        const Binding& ret = returnType->normalize(tc);
        fnType = &tc.newFunctionValue(params, ret.valueAsType());
        tc.insert(name->name, *fnType);
        body->check(tc, tc.newValue(ret));
    };

    check(0);
    tc.insert(name->name, *fnType);
    tc.unify(location, result, tc.unitValue());
}

const Binding& StatementDeclaration::infer(TypeChecker& tc)
{
    return statement->infer(tc);
}

void StatementDeclaration::check(TypeChecker& tc, const Binding& result)
{
    statement->check(tc, result);
}


// Statements

const Binding& Statement::infer(TypeChecker& tc)
{
    return tc.unitValue();
}

void EmptyStatement::check(TypeChecker&, const Binding&)
{
    // nothing to do here
}

void BlockStatement::check(TypeChecker& tc, const Binding& result)
{
    TypeChecker::UnificationScope unificationScope(tc);
    TypeChecker::Scope scope(tc);
    for (const auto& decl : declarations) {
        if (decl == declarations.back())
            decl->check(tc, result);
        else
            decl->infer(tc);
    }
}

void ReturnStatement::check(TypeChecker&, const Binding&)
{
    // TODO
}

const Binding& IfStatement::infer(TypeChecker& tc)
{
    condition->check(tc, tc.booleanValue());
    const Binding& result = consequent->infer(tc);
    if (!alternate) {
        tc.unify(location, result, tc.unitValue());
        return tc.unitValue();
    }
    (*alternate)->check(tc, result);
    return result;
}

void IfStatement::check(TypeChecker& tc, const Binding& type)
{
    condition->check(tc, tc.booleanValue());
    if (alternate) {
        consequent->check(tc, type);
        (*alternate)->check(tc, type);
    } else {
        tc.unify(location, type, tc.unitValue());
        consequent->check(tc, tc.unitValue());
    }
}

void BreakStatement::check(TypeChecker&, const Binding&)
{
    // TODO
}

void ContinueStatement::check(TypeChecker&, const Binding&)
{
    // TODO
}

void WhileStatement::check(TypeChecker&, const Binding&)
{
    // TODO
}

void ForStatement::check(TypeChecker&, const Binding&)
{
    // TODO
}

const Binding& ExpressionStatement::infer(TypeChecker& tc)
{
    return expression->infer(tc);
}

void ExpressionStatement::check(TypeChecker& tc, const Binding& result)
{
    expression->check(tc, result);
}


// Expressions

const Binding& Identifier::infer(TypeChecker& tc)
{
    return tc.lookup(location, name);
}

void Identifier::check(TypeChecker& tc, const Binding& type)
{
    // TODO: we'll eventually need something custom here
    tc.unify(location, infer(tc), type);
}

const Binding& BinaryExpression::infer(TypeChecker& tc)
{
    // TODO
    return tc.unitValue();
}

void BinaryExpression::check(TypeChecker&, const Binding&)
{
    // TODO
}

const Binding& ParenthesizedExpression::infer(TypeChecker& tc)
{
    return expression->infer(tc);
}

void ParenthesizedExpression::check(TypeChecker& tc, const Binding& type)
{
    expression->check(tc, type);
}

const Binding& ObjectLiteralExpression::infer(TypeChecker& tc)
{
    Fields typeFields;
    for (auto& pair : fields) {
        typeFields.emplace(pair.first->name, pair.second->infer(tc));
    }
    return tc.newRecordValue(typeFields);
}

void ObjectLiteralExpression::check(TypeChecker& tc, const Binding& type)
{
    if (!type.type().is<TypeRecord>()) {
        infer(tc); // check for errors in the type anyhow
        tc.typeError(location, "Unexpected record type");
        return;
    }

    // TODO: actual checking
}

const Binding& ArrayLiteralExpression::infer(TypeChecker& tc)
{
    const Binding& itemBinding = items.size()
        ? items[0]->infer(tc)
        : tc.unitValue();

    for (uint32_t i = 1; i < items.size(); i++)
        items[i]->check(tc, itemBinding);

    return tc.newArrayValue(itemBinding.type());
}

void ArrayLiteralExpression::check(TypeChecker& tc, const Binding& type)
{
    if (!type.type().is<TypeArray>()) {
        tc.typeError(location, "Unexpected array");
        infer(tc); // check for error in the array anyhow
        return;
    }

    const Type& itemType = type.type().as<TypeArray>().itemType();
    const Binding& itemBinding = tc.newValue(itemType);
    for (const auto& item : items)
        item->check(tc, itemBinding);
}

const Binding& CallExpression::infer(TypeChecker& tc)
{
    TypeChecker::UnificationScope scope(tc);
    const Binding& calleeBinding = callee->infer(tc);
    const Type& calleeType = calleeBinding.type();
    if (calleeType.is<TypeBottom>()) {
        for (const auto& arg : arguments)
            arg->infer(tc); // check arguments are well-formed
        return tc.bottomValue();
    }

    if (!calleeType.is<TypeFunction>()) {
        tc.typeError(location, "Callee is not a function");
        return tc.unitValue();
    }

    const TypeFunction& calleeTypeFunction = calleeType.as<TypeFunction>();
    if (calleeTypeFunction.paramCount() != arguments.size())
        tc.typeError(location, "Argument count mismatch");
    else
        for (unsigned i = 0; i < arguments.size(); i++)
            arguments[i]->check(tc, calleeTypeFunction.param(i));
    const Type& result = scope.result(calleeTypeFunction.returnType());
    return tc.newValue(result);
}

void CallExpression::check(TypeChecker& tc, const Binding& type)
{
    TypeChecker::UnificationScope scope(tc);
    const Binding& calleeBinding = callee->infer(tc);
    const Type& calleeType = calleeBinding.type();
    if (calleeType.is<TypeBottom>()) {
        for (const auto& arg : arguments)
            arg->infer(tc); // check arguments are well-formed
        return;
    }

    if (!calleeType.is<TypeFunction>()) {
        tc.typeError(location, "Callee is not a function");
        return;
    }

    const TypeFunction& calleeTypeFunction = calleeType.as<TypeFunction>();
    if (calleeTypeFunction.paramCount() != arguments.size())
        tc.typeError(location, "Argument count mismatch");
    else
        for (unsigned i = 0; i < arguments.size(); i++)
            arguments[i]->check(tc, calleeTypeFunction.param(i));
    tc.unify(location, tc.newValue(calleeTypeFunction.returnType()), type);
}

const Binding& SubscriptExpression::infer(TypeChecker& tc)
{
    const Binding& targetBinding = target->infer(tc);
    const Type& targetType = targetBinding.type();
    if (!targetType.is<TypeArray>()) {
        tc.typeError(location, "Trying to subscript non-array");
        return tc.unitValue();
    }

    const TypeArray& targetArrayType = targetType.as<TypeArray>();
    index->check(tc, tc.numericValue());
    return tc.newValue(targetArrayType.itemType());
}

void SubscriptExpression::check(TypeChecker& tc, const Binding& itemType)
{
    index->check(tc, tc.numericValue());
    target->check(tc, tc.newArrayValue(itemType.type()));
}

const Binding& MemberExpression::infer(TypeChecker& tc)
{
    const Binding& targetBinding = object->infer(tc);
    const Type& targetType = targetBinding.type();
    if (!targetType.is<TypeRecord>()) {
        tc.typeError(location, "Trying to access field of non-record");
        return tc.unitValue();
    }

    const TypeRecord& recordType = targetType.as<TypeRecord>();
    auto optionalFieldType = recordType.field(property->name);
    if (!optionalFieldType) {
        tc.typeError(location, "Unknown field");
        return tc.unitValue();
    }

    return *optionalFieldType;
}

void MemberExpression::check(TypeChecker& tc, const Binding& type)
{
    // TODO: do we need something custom here?
    tc.unify(location, infer(tc), type);
}

const Binding& LiteralExpression::infer(TypeChecker& tc)
{
    return literal->infer(tc);
}

void LiteralExpression::check(TypeChecker& tc, const Binding& result)
{
    tc.unify(location, infer(tc), result);
}


// Literals

const Binding& BooleanLiteral::infer(TypeChecker& tc)
{
    return tc.booleanValue();
}

const Binding& NumericLiteral::infer(TypeChecker& tc)
{
    return tc.numericValue();
}

const Binding& StringLiteral::infer(TypeChecker& tc)
{
    return tc.stringValue();
}

// Types
const Binding& TypedIdentifier::normalize(TypeChecker& tc)
{
    const Binding& binding = type->normalize(tc);
    if (binding.valueAsType().is<TypeType>())
        return tc.newVarType(name->name);
    else
        return tc.newValue(binding.valueAsType());
}

const Binding& ASTTypeName::normalize(TypeChecker& tc)
{
    return tc.lookup(location, name->name);
}

const Binding& ASTTypeType::normalize(TypeChecker& tc)
{
    return tc.typeType();
}
