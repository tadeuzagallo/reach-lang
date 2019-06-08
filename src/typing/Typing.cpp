#include "AST.h"
#include "Type.h"
#include "TypeChecker.h"


template<typename T>
void TypeChecker::inferAsType(const T& node, Register result)
{
    node->generate(m_generator, result);

    unify(node->location, result, typeType());
    //if (!result.valueIsType())
        //return unitType();
    //return result;
}


// Declarations

void Declaration::infer(TypeChecker& tc, Register result)
{
    check(tc, tc.unitType());
}

void LexicalDeclaration::check(TypeChecker& tc, Register type)
{
    Register tmp = tc.generator().newLocal();
    (*initializer)->infer(tc, tmp);
    tc.insert(name->name, tmp);
    tc.newValue(tmp, type);
    tc.unify(location, tmp, tc.unitType());
}

void FunctionDeclaration::check(TypeChecker& tc, Register result)
{
    valueRegister = std::make_unique<Register>(tc.generator().newLocal());
    Register typeRegister = tc.generator().newLocal();

    std::vector<Register> parameterRegisters;
    Register returnRegister = tc.generator().newLocal();
    for (const auto& _ : parameters)
        parameterRegisters.emplace_back(tc.generator().newLocal());

    bool shadowsFunctionName = false;
    std::function<void(uint32_t)> check = [&](uint32_t i) {
        TypeChecker::Scope scope(tc);
        if (i < parameters.size()) {
            if (parameters[i]->name->name == name->name)
                shadowsFunctionName = true;
            parameters[i]->infer(tc, parameterRegisters[i]);
            check(i + 1);
            return;
        }

        tc.inferAsType(returnType, returnRegister);
        tc.newFunctionType(typeRegister, parameterRegisters, returnRegister);
        if (!shadowsFunctionName) {
            tc.newValue(*valueRegister, typeRegister);
            tc.insert(name->name, *valueRegister);
        }
        //tc.newValue(returnRegister, returnRegister);
        body->check(tc, returnRegister);
    };

    check(0);

    generateImpl(tc.generator(), *valueRegister, typeRegister);

    tc.insert(name->name, *valueRegister);
    Register tmp = tc.generator().newLocal();
    tc.newValue(tmp, result);
    tc.unify(location, tmp, tc.unitType());
}

void StatementDeclaration::infer(TypeChecker& tc, Register result)
{
    statement->infer(tc, result);
}

void StatementDeclaration::check(TypeChecker& tc, Register result)
{
    statement->check(tc, result);
}


// Statements

void Statement::infer(TypeChecker& tc, Register result)
{
    tc.unitValue(result);
}

void EmptyStatement::check(TypeChecker&, Register)
{
    // nothing to do here
}

void BlockStatement::check(TypeChecker& tc, Register result)
{
    TypeChecker::UnificationScope unificationScope(tc);
    TypeChecker::Scope scope(tc);
    for (const auto& decl : declarations) {
        if (decl == declarations.back())
            decl->check(tc, result);
        else {
            Register tmp = tc.generator().newLocal();
            decl->infer(tc, tmp);
        }
    }
}

void ReturnStatement::check(TypeChecker&, Register)
{
    // TODO
}

void IfStatement::infer(TypeChecker& tc, Register result)
{
    tc.booleanValue(result);
    condition->check(tc, result);
    consequent->infer(tc, result);
    if (!alternate) {
        tc.unify(location, result, tc.unitType());
        tc.unitValue(result);
    } else {
        tc.generator().getTypeForValue(result, result);
        (*alternate)->check(tc, result);
    }
}

void IfStatement::check(TypeChecker& tc, Register type)
{
    Register tmp = tc.generator().newLocal();
    tc.booleanValue(tmp);
    condition->check(tc, tmp);
    if (alternate) {
        consequent->check(tc, type);
        (*alternate)->check(tc, type);
    } else {
        tc.unify(location, type, tc.unitType());
        consequent->check(tc, tc.unitType());
    }
}

void BreakStatement::check(TypeChecker&, Register)
{
    // TODO
}

void ContinueStatement::check(TypeChecker&, Register)
{
    // TODO
}

void WhileStatement::check(TypeChecker&, Register)
{
    // TODO
}

void ForStatement::check(TypeChecker&, Register)
{
    // TODO
}

void ExpressionStatement::infer(TypeChecker& tc, Register result)
{
    expression->infer(tc, result);
}

void ExpressionStatement::check(TypeChecker& tc, Register result)
{
    expression->check(tc, result);
}


// Expressions

void Identifier::infer(TypeChecker& tc, Register result)
{
    //tc.unitValue(result);
    tc.lookup(result, location, name);
}

void Identifier::check(TypeChecker& tc, Register type)
{
    // TODO: we'll eventually need something custom here
    Register tmp = tc.generator().newLocal();
    infer(tc, tmp);
    tc.unify(location, tmp, type);
}

void BinaryExpression::infer(TypeChecker& tc, Register result)
{
    // TODO
    tc.unitValue(result);
}

void BinaryExpression::check(TypeChecker&, Register)
{
    // TODO
}

void ParenthesizedExpression::infer(TypeChecker& tc, Register result)
{
    expression->infer(tc, result);
}

void ParenthesizedExpression::check(TypeChecker& tc, Register type)
{
    expression->check(tc, type);
}

void ObjectLiteralExpression::infer(TypeChecker& tc, Register result)
{
    std::vector<std::pair<std::string, Register>> fieldRegisters;

    for (const auto& pair : fields)
        fieldRegisters.emplace_back(std::make_pair(pair.first->name, tc.generator().newLocal()));

    unsigned i = 0;
    for (auto& pair : fields) {
        pair.second->infer(tc, fieldRegisters[i++].second);
    }

    tc.newRecordValue(result, fieldRegisters);
}

void ObjectLiteralExpression::check(TypeChecker& tc, Register type)
{
    Register tmp = tc.generator().newLocal();
    tc.generator().checkValue(tmp, type, Type::Class::Record);
    tc.generator().branch(tmp, [&] {
        // TODO: actual check
    }, [&] {
        infer(tc, type);
        tc.generator().typeError("Unexpected record");
    });
}

void ArrayLiteralExpression::infer(TypeChecker& tc, Register result)
{
    if (items.size()) {
        items[0]->infer(tc, result);
        tc.generator().getTypeForValue(result, result);
    } else
        tc.unitValue(result);

    for (uint32_t i = 1; i < items.size(); i++)
        items[i]->check(tc, result);

    tc.newArrayValue(result, result);
}

void ArrayLiteralExpression::check(TypeChecker& tc, Register type)
{
    Register tmp = tc.generator().newLocal();
    tc.generator().checkValue(tmp, type, Type::Class::Array);
    tc.generator().branch(tmp, [&]{
        tc.generator().getField(tmp, type, TypeArray::itemTypeField);
        tc.generator().newValue(tmp, tmp);
        for (const auto& item : items)
            item->check(tc, tmp);
    }, [&] {
        tc.generator().typeError("Unexpected array");
    });
}

void CallExpression::checkCallee(TypeChecker& tc, Register result, Label& isBottom)
{
    callee->infer(tc, result);
    Register tmp = tc.generator().newLocal();
    tc.generator().checkValue(tmp, result, Type::Class::Bottom);
    tc.generator().branch(tmp, [&] {
        if (Identifier* ident = dynamic_cast<Identifier*>(callee.get())) {
            if (ident->name == "inspect") {
                for (auto it = arguments.begin(); it != arguments.end(); it++) {
                    std::unique_ptr<Expression>& argument = *it++;
                    auto typeIndex = std::make_unique<uint32_t>(tc.vm().globalConstants.size());
                    tc.vm().globalConstants.push_back(Value::crash());
                    argument->infer(tc, tmp);
                    tc.generator().storeGlobalConstant(tmp, *typeIndex);
                    auto type = std::make_unique<SynthesizedTypeExpression>(argument->location);
                    type->typeIndex = std::move(typeIndex);
                    it = arguments.emplace(it, std::move(type));
                }
                goto result;
            }
        }

        // check arguments are well-formed
        for (const auto& arg : arguments)
            arg->infer(tc, tmp);
result:
        tc.bottomValue(result);
        tc.generator().jump(isBottom);
    }, [&] {
        tc.generator().checkValue(tmp, result, Type::Class::Function);
        tc.generator().branch(tmp, [&] {
            tc.generator().getTypeForValue(result, result);
        }, [&] {
            tc.generator().typeError("Callee is not a function");
        });
    });
}

void CallExpression::checkArguments(TypeChecker& tc, Register calleeType, TypeChecker::UnificationScope& scope)
{
    Register explicitParamCount = tc.generator().newLocal();
    tc.generator().getField(explicitParamCount, calleeType, TypeFunction::explicitParamCountField);

    Register tmp = tc.generator().newLocal();
    tc.generator().loadConstant(tmp, static_cast<uint32_t>(arguments.size()));
    tc.generator().isEqual(tmp, tmp, explicitParamCount);
    tc.generator().branch(tmp, [&] {}, [&] {
        // TODO: location
        tc.generator().typeError("Argument count mismatch");
    });

    Register param = tc.generator().newLocal();
    Register index = tc.generator().newLocal();
    tc.generator().getField(tmp, calleeType, TypeFunction::explicitParamsField);
    for (uint32_t i = 0; i < arguments.size(); i++) {
        tc.generator().loadConstant(index, i);
        tc.generator().getArrayIndex(param, tmp, index);
        arguments[i]->check(tc, param);
    }
    tc.generator().inferImplicitParameters(calleeType);

    //auto it = arguments.begin();
    //for (uint32_t i = 0; i < calleeTypeFunction.paramCount(); i++) {
        //Register param = calleeTypeFunction.param(i);
        //if (!param.inferred()) {
            //++it;
            //continue;
        //}

        //const Type& inferredType = scope.resolve(param.valueAsType());
        //auto argument = std::make_unique<SynthesizedTypeExpression>(location);
        //it = ++arguments.emplace(it, std::move(argument));
    //}
    //ASSERT(it == arguments.end(), "");
}

void CallExpression::infer(TypeChecker& tc, Register result)
{
    TypeChecker::UnificationScope scope(tc);
    Label isBottom = tc.generator().label();
    checkCallee(tc, result, isBottom);
    checkArguments(tc, result, scope);
    tc.generator().getField(result, result, TypeFunction::returnTypeField);
    scope.resolve(result, result);
    tc.newValue(result, result);
    tc.generator().emit(isBottom);
}

void CallExpression::check(TypeChecker& tc, Register type)
{
    // TODO
    //TypeChecker::UnificationScope scope(tc);
    //const TypeFunction* calleeTypeFunction = checkCallee(tc);
    //if (!calleeTypeFunction)
        //return;
    //tc.unify(location, tc.newValue(calleeTypeFunction->returnType()), type);
    //checkArguments(tc, *calleeTypeFunction, scope);
}

void SubscriptExpression::infer(TypeChecker& tc, Register result)
{
    target->infer(tc, result);

    Register tmp = tc.generator().newLocal();
    index->check(tc, tc.numberType());

    tc.generator().checkValue(tmp, result, Type::Class::Array);
    tc.generator().branch(tmp, [&] {
        tc.generator().getTypeForValue(result, result);
        tc.generator().getField(result, result, TypeArray::itemTypeField);
        tc.generator().newValue(result, result);
    }, [&] {
        tc.generator().typeError("Trying to subscript non-array");
    });


}

void SubscriptExpression::check(TypeChecker& tc, Register itemType)
{
    //Register tmp = tc.generator().newLocal();
    //index->check(tc, tc.numberType());
    //tc.generator().newArrayValue(itemType);
    //target->check(tc, itemType);
}

void MemberExpression::infer(TypeChecker& tc, Register result)
{
    // FIXME: should check tc against { this->field }
    Register tmp = tc.generator().newLocal();
    object->infer(tc, result);
    tc.generator().checkValue(tmp, result, Type::Class::Record);
    tc.generator().branch(tmp, [&] {
        tc.generator().getTypeForValue(result, result);
        tc.generator().getField(result, result, TypeRecord::fieldsField);
        tc.generator().getField(result, result, property->name);
    }, [&] {
        tc.generator().typeError("Trying to acess field of non-record");
    });
}

void MemberExpression::check(TypeChecker& tc, Register result)
{
    // TODO: do we need something custom here?
    //Register tmp = tc.generator().newLocal();
    //infer(tc, tmp);
    //tc.unify(location, tmp, type);
}

void LiteralExpression::infer(TypeChecker& tc, Register result)
{
    literal->infer(tc, result);
}

void LiteralExpression::check(TypeChecker& tc, Register result)
{
    Register tmp = tc.generator().newLocal();
    infer(tc, tmp);
    tc.unify(location, tmp, result);
}

void TypeExpression::infer(TypeChecker& tc, Register result)
{
    type->infer(tc, result);
}
void TypeExpression::check(TypeChecker& tc, Register result)
{
    Register tmp = tc.generator().newLocal();
    tc.typeType(tmp);
    tc.unify(location, result, tmp);
    infer(tc, tmp);
    tc.unify(location, tmp, result);
}

void SynthesizedTypeExpression::infer(TypeChecker& tc, Register)
{
    tc.generator().typeError("Should not infer type for SynthesizedTypeExpression");
}

void SynthesizedTypeExpression::check(TypeChecker& tc, Register)
{
    tc.generator().typeError("Should not type check SynthesizedTypeExpression");
}


// Literals

void BooleanLiteral::infer(TypeChecker& tc, Register result)
{
    tc.booleanValue(result);
}

void NumericLiteral::infer(TypeChecker& tc, Register result)
{
    return tc.numberValue(result);
}

void StringLiteral::infer(TypeChecker& tc, Register result)
{
    return tc.stringValue(result);
}

// Types
void TypedIdentifier::infer(TypeChecker& tc, Register result)
{
    type->check(tc, tc.typeType());
    type->generate(tc.generator(), result);

    Register tmp = tc.generator().newLocal();
    tc.generator().checkType(tmp, result, Type::Class::Type);
    tc.generator().branch(tmp, [&]{
        tc.generator().newVarType(result, name->name, inferred);
        tc.insert(name->name, result);
    }, [&]{
        if (inferred)
            tc.generator().typeError("Only type arguments can be inferred");
        tc.generator().newValue(tmp, result);
        tc.insert(name->name, tmp);
    });
}

void ASTTypeType::infer(TypeChecker& tc, Register result)
{
    tc.typeType(result);
}
