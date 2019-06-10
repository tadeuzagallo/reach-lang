#include "AST.h"
#include "Type.h"
#include "TypeChecker.h"


template<typename T>
void TypeChecker::inferAsType(T& node, Register result)
{
    node->check(*this, typeType());
    node->generate(m_generator, result);
}


// Declarations

void Declaration::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    check(tc, tc.unitType());
}

void LexicalDeclaration::check(TypeChecker& tc, Register type)
{
    tc.generator().emitLocation(location);
    Register tmp = tc.generator().newLocal();
    (*initializer)->infer(tc, tmp);
    tc.insert(name->name, tmp);
    tc.newValue(tmp, type);
    tc.unify(location, tmp, tc.unitType());
}

void FunctionDeclaration::check(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    Register valueRegister = tc.generator().newLocal();

    BytecodeGenerator functionGenerator { tc.generator().vm(), name->name };
    TypeChecker functionTC { functionGenerator };

    Register resultRegister = functionGenerator.newLocal();
    Register typeRegister = functionGenerator.newLocal();

    std::vector<Register> parameterRegisters;
    Register returnRegister = functionGenerator.newLocal();
    for (const auto& _ : parameters)
        parameterRegisters.emplace_back(functionGenerator.newLocal());

    bool shadowsFunctionName = false;
    std::function<void(uint32_t)> check = [&](uint32_t i) {
        TypeChecker::Scope scope(functionTC);
        if (i < parameters.size()) {
            if (parameters[i]->name->name == name->name)
                shadowsFunctionName = true;
            parameters[i]->infer(functionTC, parameterRegisters[i]);
            check(i + 1);
            return;
        }

        functionTC.inferAsType(returnType, returnRegister);
        functionTC.newFunctionType(typeRegister, parameterRegisters, returnRegister);
        if (!shadowsFunctionName) {
            functionTC.newValue(resultRegister, typeRegister);
            functionTC.insert(name->name, resultRegister);
        }
        body->check(functionTC, returnRegister);
    };

    check(0);

    functionGenerator.endTypeChecking(typeRegister);
    generateImpl(functionGenerator, resultRegister);
    auto block = functionTC.finalize(resultRegister);
    functionIndex = tc.generator().newFunction(valueRegister, std::move(block));
    tc.insert(name->name, valueRegister);

    Register tmp = tc.generator().newLocal();
    tc.newValue(tmp, result);
    tc.unify(location, tmp, tc.unitType());
}

void StatementDeclaration::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    statement->infer(tc, result);
}

void StatementDeclaration::check(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    statement->check(tc, result);
}


// Statements

void Statement::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    tc.unitValue(result);
}

void EmptyStatement::check(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);
    // nothing to do here
}

void BlockStatement::check(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
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

void ReturnStatement::check(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);
    // TODO
}

void IfStatement::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    condition->check(tc, tc.boolType());
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
    tc.generator().emitLocation(location);
    Register tmp = tc.generator().newLocal();
    condition->check(tc, tc.boolType());
    if (alternate) {
        consequent->check(tc, type);
        (*alternate)->check(tc, type);
    } else {
        tc.unify(location, type, tc.unitType());
        consequent->check(tc, tc.unitType());
    }
}

void BreakStatement::check(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);
    // TODO
}

void ContinueStatement::check(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);
    // TODO
}

void WhileStatement::check(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);
    // TODO
}

void ForStatement::check(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);
    // TODO
}

void ExpressionStatement::infer(TypeChecker& tc, Register result)
{
    expression->infer(tc, result);
}

void ExpressionStatement::check(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    expression->check(tc, result);
}


// Expressions

void Identifier::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    // TODO: handle unknown variables
    tc.lookup(result, location, name);
}

void Identifier::check(TypeChecker& tc, Register type)
{
    tc.generator().emitLocation(location);
    // TODO: we'll eventually need something custom here
    Register tmp = tc.generator().newLocal();
    infer(tc, tmp);
    tc.unify(location, tmp, type);
}

void BinaryExpression::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    // TODO
    tc.unitValue(result);
}

void BinaryExpression::check(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);
    // TODO
}

void ParenthesizedExpression::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);
    expression->infer(tc, result);
}

void ParenthesizedExpression::check(TypeChecker& tc, Register type)
{
    tc.generator().emitLocation(location);
    expression->check(tc, type);
}

void ObjectLiteralExpression::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    std::vector<std::pair<std::string, Register>> fieldRegisters;

    for (const auto& pair : fields)
        fieldRegisters.emplace_back(std::make_pair(pair.first->name, tc.generator().newLocal()));

    unsigned i = 0;
    for (auto& pair : fields) {
        Register type = fieldRegisters[i++].second;
        pair.second->infer(tc, type);
        //tc.generator().getTypeForValue(type, type);
    }

    tc.newRecordValue(result, fieldRegisters);
}

void ObjectLiteralExpression::check(TypeChecker& tc, Register type)
{
    tc.generator().emitLocation(location);

    Register tmp = tc.generator().newLocal();
    tc.generator().checkType(tmp, type, Type::Class::Record);
    tc.generator().branch(tmp, [&] {
        // TODO: actual check
    }, [&] {
        infer(tc, type);
        tc.generator().typeError("Unexpected record");
    });
}

void ArrayLiteralExpression::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    Register tmp = tc.generator().newLocal();
    if (items.size()) {
        items[0]->infer(tc, result);
        tc.generator().getTypeForValue(tmp, result);
    } else
        tc.unitValue(result);

    for (uint32_t i = 1; i < items.size(); i++)
        items[i]->check(tc, tmp);

    tc.newArrayValue(result, result);
}

void ArrayLiteralExpression::check(TypeChecker& tc, Register type)
{
    tc.generator().emitLocation(location);

    Register tmp = tc.generator().newLocal();
    tc.generator().checkValue(tmp, type, Type::Class::Array);
    tc.generator().branch(tmp, [&]{
        tc.generator().getField(tmp, type, TypeArray::itemTypeField);
        for (const auto& item : items)
            item->check(tc, tmp);
    }, [&] {
        tc.generator().typeError("Unexpected array");
    });
}

void CallExpression::checkCallee(TypeChecker& tc, Register result, Label& done)
{
    callee->infer(tc, result);
    Register tmp = tc.generator().newLocal();
    tc.generator().checkValue(tmp, result, Type::Class::Bottom);
    tc.generator().branch(tmp, [&] {
        if (Identifier* ident = dynamic_cast<Identifier*>(callee.get())) {
            if (ident->name == "inspect") {
                for (auto it = arguments.begin(); it != arguments.end(); it++) {
                    std::unique_ptr<Expression>& argument = *it++;
                    argument->infer(tc, tmp);
                    auto type = std::make_unique<SynthesizedTypeExpression>(argument->location);
                    type->typeIndex = tc.generator().storeConstant(tmp);
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
        tc.generator().jump(done);
    }, [&] {
        tc.generator().checkValue(tmp, result, Type::Class::Function);
        tc.generator().branch(tmp, [&] {
            tc.generator().getTypeForValue(result, result);
        }, [&] {
            tc.generator().typeError("Callee is not a function");
            tc.generator().jump(done);
        });
    });
}

void CallExpression::checkArguments(TypeChecker& tc, Register calleeType, TypeChecker::UnificationScope& scope, Label& done)
{
    Register explicitParamCount = tc.generator().newLocal();
    tc.generator().getField(explicitParamCount, calleeType, TypeFunction::explicitParamCountField);

    Register tmp = tc.generator().newLocal();
    tc.generator().loadConstant(tmp, static_cast<uint32_t>(arguments.size()));
    tc.generator().isEqual(tmp, tmp, explicitParamCount);
    tc.generator().branch(tmp, [&] {}, [&] {
        tc.generator().typeError("Argument count mismatch");
        tc.unitValue(calleeType);
        tc.generator().jump(done);
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
    tc.generator().emitLocation(location);

    TypeChecker::UnificationScope scope(tc);
    Label done = tc.generator().label();
    checkCallee(tc, result, done);
    checkArguments(tc, result, scope, done);
    tc.generator().getField(result, result, TypeFunction::returnTypeField);
    scope.resolve(result, result);
    tc.newValue(result, result);
    tc.generator().emit(done);
}

void CallExpression::check(TypeChecker& tc, Register type)
{
    tc.generator().emitLocation(location);

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
    tc.generator().emitLocation(location);

    target->infer(tc, result);

    Register tmp = tc.generator().newLocal();
    index->check(tc, tc.numberType());

    tc.generator().checkValue(tmp, result, Type::Class::Array);
    tc.generator().branch(tmp, [&] {
        tc.generator().getTypeForValue(result, result);
        tc.generator().getField(result, result, TypeArray::itemTypeField);
    }, [&] {
        tc.generator().typeError("Trying to subscript non-array");
    });


}

void SubscriptExpression::check(TypeChecker& tc, Register itemType)
{
    tc.generator().emitLocation(location);

    //Register tmp = tc.generator().newLocal();
    //index->check(tc, tc.numberType());
    //tc.generator().newArrayValue(itemType);
    //target->check(tc, itemType);
}

void MemberExpression::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

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
    tc.generator().emitLocation(location);

    // TODO: do we need something custom here?
    //Register tmp = tc.generator().newLocal();
    //infer(tc, tmp);
    //tc.unify(location, tmp, type);
}

void LiteralExpression::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    literal->infer(tc, result);
}

void LiteralExpression::check(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    Register tmp = tc.generator().newLocal();
    infer(tc, tmp);
    tc.unify(location, tmp, result);
}

void TypeExpression::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    type->infer(tc, result);
}
void TypeExpression::check(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    Register tmp = tc.generator().newLocal();
    tc.generator().move(tmp, tc.typeType());
    tc.unify(location, result, tmp);
    infer(tc, tmp);
    tc.unify(location, tmp, result);
}

void SynthesizedTypeExpression::infer(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);

    tc.generator().typeError("Should not infer type for SynthesizedTypeExpression");
}

void SynthesizedTypeExpression::check(TypeChecker& tc, Register)
{
    tc.generator().emitLocation(location);

    tc.generator().typeError("Should not type check SynthesizedTypeExpression");
}


// Literals

void BooleanLiteral::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    tc.boolValue(result);
}

void NumericLiteral::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    return tc.numberValue(result);
}

void StringLiteral::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    return tc.stringValue(result);
}

// Types
void TypedIdentifier::infer(TypeChecker& tc, Register result)
{
    tc.generator().emitLocation(location);

    tc.inferAsType(type, result);

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
    tc.generator().emitLocation(location);

    tc.generator().move(result, tc.typeType());
}
