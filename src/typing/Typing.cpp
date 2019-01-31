#include "AST.h"
#include "Binding.h"
#include "Type.h"
#include "TypeChecker.h"


template<typename T>
const Binding& TypeChecker::inferAsType(const T& node)
{
    const Binding& result = node->infer(*this);
    unify(node->location, result, typeType());
    if (!result.valueIsType())
        return unitType();
    return result;
}

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
            const Binding& binding = parameters[i]->infer(tc);
            params.emplace_back(binding);
            tc.insert(parameters[i]->name->name, binding);
            check(i + 1);
            return;
        }

        const Binding& ret = tc.inferAsType(returnType);
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
    const Binding& result = statement->infer(tc);
    binding = statement->binding = &tc.newType(result);
    return result;
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
    const Binding& result = expression->infer(tc);
    binding = expression->binding = &tc.newType(result);
    return result;
}

void ExpressionStatement::check(TypeChecker& tc, const Binding& result)
{
    expression->check(tc, result);
}


// Expressions

const Binding& Identifier::infer(TypeChecker& tc)
{
    const Binding& result = tc.lookup(location, name, tc.unitValue());
    binding = &tc.newType(result);
    return result;
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

    binding = &tc.newRecordType(typeFields);
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

    binding = &tc.newArrayType(itemBinding.type());
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

const TypeFunction* CallExpression::checkCallee(TypeChecker& tc, const Binding*& binding)
{
    const Binding& calleeBinding = callee->infer(tc);
    const Type& calleeType = calleeBinding.type();
    if (calleeType.is<TypeBottom>()) {
        for (const auto& arg : arguments)
            arg->infer(tc); // check arguments are well-formed
        binding = &tc.bottomValue();
        return nullptr;
    }

    if (!calleeType.is<TypeFunction>()) {
        tc.typeError(location, "Callee is not a function");
        binding = &tc.unitValue();
        return nullptr;
    }

    return &calleeType.as<TypeFunction>();
}

void CallExpression::checkArguments(TypeChecker& tc, const TypeFunction& calleeTypeFunction, TypeChecker::UnificationScope& scope)
{
    bool argumentMismatch = calleeTypeFunction.explicitParamCount() != arguments.size();
    if (argumentMismatch)
        tc.typeError(location, "Argument count mismatch");
    else {
        uint32_t argIndex = 0;
        for (uint32_t i = 0; i < calleeTypeFunction.paramCount(); i++) {
            const Binding& param = calleeTypeFunction.param(i);
            if (param.inferred())
                scope.infer(location, param);
            else
                arguments[argIndex++]->check(tc, param);
        }
        ASSERT(argIndex == arguments.size(), "");

        auto it = arguments.begin();
        for (uint32_t i = 0; i < calleeTypeFunction.paramCount(); i++) {
            const Binding& param = calleeTypeFunction.param(i);
            if (!param.inferred()) {
                ++it;
                continue;
            }

            const Type& inferredType = scope.resolve(param.valueAsType());
            auto argument = std::make_unique<SynthesizedTypeExpression>(location);
            argument->binding = &tc.newType(inferredType);
            it = ++arguments.emplace(it, std::move(argument));
        }
        ASSERT(it == arguments.end(), "");
    }
}

const Binding& CallExpression::infer(TypeChecker& tc)
{
    TypeChecker::UnificationScope scope(tc);
    const Binding* calleeBinding;
    const TypeFunction* calleeTypeFunction = checkCallee(tc, calleeBinding);
    if (!calleeTypeFunction)
        return *calleeBinding;
    checkArguments(tc, *calleeTypeFunction, scope);
    const Type& result = scope.resolve(calleeTypeFunction->returnType());
    binding = &tc.newType(result);
    return tc.newValue(result);
}

void CallExpression::check(TypeChecker& tc, const Binding& type)
{
    TypeChecker::UnificationScope scope(tc);
    const Binding* calleeBinding;
    const TypeFunction* calleeTypeFunction = checkCallee(tc, calleeBinding);
    if (!calleeTypeFunction)
        return;
    tc.unify(location, tc.newValue(calleeTypeFunction->returnType()), type);
    checkArguments(tc, *calleeTypeFunction, scope);
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

    binding = &tc.newType(targetArrayType.itemType());
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

    binding = &tc.newType(*optionalFieldType);
    return *optionalFieldType;
}

void MemberExpression::check(TypeChecker& tc, const Binding& type)
{
    // TODO: do we need something custom here?
    tc.unify(location, infer(tc), type);
}

const Binding& LiteralExpression::infer(TypeChecker& tc)
{
    const Binding& result = literal->infer(tc);
    binding = literal->binding = &tc.newType(result);
    return result;
}

void LiteralExpression::check(TypeChecker& tc, const Binding& result)
{
    tc.unify(location, infer(tc), result);
}

const Binding& TypeExpression::infer(TypeChecker& tc)
{
    return type->infer(tc);
}
void TypeExpression::check(TypeChecker& tc, const Binding& result)
{
    tc.unify(location, result, tc.typeType());
    tc.unify(location, infer(tc), result);
}

const Binding& SynthesizedTypeExpression::infer(TypeChecker& tc)
{
    ASSERT(false, "Should not infer type for SynthesizedTypeExpression");
    return tc.unitValue();
}

void SynthesizedTypeExpression::check(TypeChecker&, const Binding&)
{
    ASSERT(false, "Should not type check SynthesizedTypeExpression");
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
const Binding& TypedIdentifier::infer(TypeChecker& tc)
{
    const Binding& binding = tc.inferAsType(type);
    if (binding.valueAsType().is<TypeType>())
        return tc.newVarType(name->name, inferred);
    else {
        if (inferred)
            tc.typeError(location, "Only type arguments can be inferred");
        return tc.newValue(binding.valueAsType());
    }
}

const Binding& ASTTypeType::infer(TypeChecker& tc)
{
    return tc.typeType();
}
