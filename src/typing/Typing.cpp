#include "AST.h"
#include "Type.h"
#include "TypeChecker.h"


template<typename T>
void TypeChecker::inferAsType(T& node, Register result)
{
    node->check(*this, typeType());
    node->generateForTypeChecking(*this, result);

    Register tmp = generator().newLocal();
    generator().checkType(tmp, result, Type::Class::AnyType);
    generator().branch(tmp, [&] { }, [&] {
        // Recover, since we already unified and type errors will already be emitted
        generator().move(result, topType());
    });
}

// Declarations

void Declaration::infer(TypeChecker& tc, Register result)
{
    check(tc, tc.unitType());

    tc.unitValue(result);
}

void LexicalDeclaration::check(TypeChecker& tc, Register type)
{
    Register tmp = tc.generator().newLocal();
    Register initType = tc.generator().newLocal();
    if (this->type) {
        tc.inferAsType(this->type, initType);
        initializer->check(tc, initType);
        tc.generator().newValue(initType, initType);
    } else
        initializer->infer(tc, initType);

    tc.insert(name->name, initType);
    tc.newValue(tmp, type);
    tc.unify(location, tmp, tc.unitType());
}

void FunctionDeclaration::check(TypeChecker& tc, Register result)
{
    tc.currentScope().addFunction(*this);

    Register valueRegister = tc.generator().newLocal();

    BytecodeGenerator functionGenerator { tc.generator().vm(), name->name };
    Register resultRegister = functionGenerator.newLocal();
    {
        TypeChecker functionTC { functionGenerator };
        Register typeRegister = functionGenerator.newLocal();

        std::vector<Register> parameterRegisters;
        Register returnRegister = functionGenerator.newLocal();
        for (uint32_t i = 0; i < parameters.size(); i++)
            parameterRegisters.emplace_back(functionGenerator.newLocal());

        uint32_t inferredParameters = 0;
        ASSERT(parameters.size() < 32, "OOPS");
        bool shadowsFunctionName = false;
        std::function<void(uint32_t)> check = [&](uint32_t i) {
            TypeChecker::Scope scope(functionTC);
            if (i < parameters.size()) {
                auto& parameter = parameters[i];
                if (parameter->name->name == name->name)
                    shadowsFunctionName = true;
                if (parameter->inferred)
                    inferredParameters |= 1 << i;
                else if (!dynamic_cast<TypeTypeExpression*>(parameter->type.get()))
                    scope.bindParameter(parameter->name->name);
                parameter->infer(functionTC, parameterRegisters[parameters.size() - i - 1]);
                check(i + 1);
                return;
            }

            functionTC.inferAsType(returnType, returnRegister);
            functionTC.generator().newFunctionType(typeRegister, parameterRegisters, returnRegister, inferredParameters);
            if (!shadowsFunctionName) {
                functionTC.newValue(resultRegister, typeRegister);
                functionTC.insert(name->name, resultRegister);
            }
            body->check(functionTC, returnRegister);
        };

        check(0);

        functionTC.endTypeChecking(TypeChecker::Mode::Function, typeRegister);
    }
    generateImpl(functionGenerator, resultRegister);
    auto block = functionGenerator.finalize(resultRegister);
    functionIndex = tc.generator().newFunction(valueRegister, std::move(block));
    tc.insert(name->name, valueRegister);

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
    ASSERT_NOT_REACHED();
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
    ASSERT_NOT_REACHED();
}

void IfStatement::infer(TypeChecker& tc, Register result)
{
    condition->check(tc, tc.boolType());
    consequent->infer(tc, result);
    tc.generator().getTypeForValue(result, result);
    Register rhs = Register::invalid();
    if (!alternate) {
        rhs = tc.unitType();
    } else {
        rhs = tc.generator().newLocal();
        (*alternate)->infer(tc, rhs);
        tc.generator().getTypeForValue(rhs, rhs);
    }
    tc.generator().newUnionType(result, result, rhs);
    tc.generator().newValue(result, result);
}

void IfStatement::check(TypeChecker& tc, Register type)
{
    bool shouldRefine = false;
    Register constantTmp = Register::invalid();
    Register conditionType = Register::invalid();
    if (auto* condition_ = dynamic_cast<InferredExpression*>(condition.get())) {
        shouldRefine = true;
        constantTmp = tc.generator().newLocal();
        conditionType = tc.generator().newLocal();
        condition_->infer(tc, conditionType);
        tc.generator().getTypeForValue(conditionType, conditionType);
    }

    const auto& refine = [&](bool value, const auto& functor) {
        if (!shouldRefine) {
            functor();
            return;
        }

        TypeChecker::UnificationScope scope(tc);
        tc.generator().loadConstant(constantTmp, value);
        tc.unify(consequent->location, constantTmp, conditionType);
        functor();
    };

    condition->check(tc, tc.boolType());
    if (alternate) {
        refine(true, [&] { consequent->check(tc, type); });
        refine(false, [&] { (*alternate)->check(tc, type); });
    } else {
        tc.unify(location, type, tc.unitType());
        refine(true, [&] { consequent->check(tc, tc.unitType()); });
    }
}

void BreakStatement::check(TypeChecker&, Register)
{
    ASSERT_NOT_REACHED();
}

void ContinueStatement::check(TypeChecker&, Register)
{
    ASSERT_NOT_REACHED();
}

void WhileStatement::check(TypeChecker&, Register)
{
    ASSERT_NOT_REACHED();
}

void ForStatement::check(TypeChecker&, Register)
{
    ASSERT_NOT_REACHED();
}

void MatchStatement::infer(TypeChecker& tc, Register result)
{
    Register scrutineeType = tc.generator().newLocal();
    Register tmp = tc.generator().newLocal();
    scrutinee->infer(tc, scrutineeType);
    for (uint32_t i = 0; i < cases.size(); i++) {
        // TODO: add unification scope here for dependent pattern matching
        auto& kase = cases[i];
        kase->pattern->infer(tc, tmp);
        tc.generator().getTypeForValue(tmp, tmp);
        tc.match(kase->pattern->location, scrutineeType, tmp);
        if (!i) {
            kase->statement->infer(tc, result);
            tc.generator().getTypeForValue(result, result);
            continue;
        }
        kase->statement->infer(tc, tmp);
        tc.generator().getTypeForValue(tmp, tmp);
        tc.generator().newUnionType(result, result, tmp);
    }
    if (defaultCase) {
        // TODO: add unification scope here for dependent pattern matching
        if (cases.size()) {
            defaultCase->infer(tc, tmp);
            tc.generator().getTypeForValue(tmp, tmp);
            tc.generator().newUnionType(result, result, tmp);
        } else {
            defaultCase->infer(tc, result);
            tc.generator().getTypeForValue(result, result);
        }
    }
    tc.generator().resolveType(result, result);
    tc.generator().newValue(result, result);
}

void MatchStatement::check(TypeChecker& tc, Register type)
{
    Register scrutineeType = tc.generator().newLocal();
    Register tmp = tc.generator().newLocal();
    scrutinee->infer(tc, scrutineeType);
    for (auto& kase : cases) {
        kase->pattern->infer(tc, tmp);
        tc.generator().getTypeForValue(tmp, tmp);
        tc.match(kase->pattern->location, scrutineeType, tmp);
        kase->statement->check(tc, type);
    }
    if (defaultCase)
        defaultCase->check(tc, type);
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

void InferredExpression::check(TypeChecker& tc, Register type)
{
    Register tmp = tc.generator().newLocal();
    infer(tc, tmp);
    tc.unify(location, tmp, type);
}

void Identifier::infer(TypeChecker& tc, Register result)
{
    tc.lookup(result, location, name);
}

void ParenthesizedExpression::infer(TypeChecker& tc, Register result)
{
    expression->infer(tc, result);
}

void LazyExpression::infer(TypeChecker& tc, Register result)
{
    expression->infer(tc, result);
}

void TupleExpression::infer(TypeChecker& tc, Register result)
{
    generateForTypeChecking(tc, result);
}

void CallExpression::checkCallee(TypeChecker& tc, Register result, Label& done)
{
    callee->infer(tc, result);
    Register tmp = tc.generator().newLocal();
    tc.generator().checkTypeOf(tmp, result, Type::Class::Function);
    tc.generator().branch(tmp, [&] {
        tc.generator().getTypeForValue(result, result);
    }, [&] {
        tc.generator().typeError(location, "Callee is not a function");
        tc.generator().jump(done);
    });
}

void CallExpression::checkArguments(TypeChecker& tc, Register calleeType, Label& done)
{
    Register explicitParamCount = tc.generator().newLocal();
    tc.generator().getField(explicitParamCount, calleeType, TypeFunction::explicitParamCountField);

    Register tmp = tc.generator().newLocal();
    tc.generator().loadConstant(tmp, static_cast<uint32_t>(arguments.size()));
    tc.generator().isEqual(tmp, tmp, explicitParamCount);
    tc.generator().branch(tmp, [&] {}, [&] {
        tc.generator().typeError(location, "Argument count mismatch");
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

    if (Identifier* ident = dynamic_cast<Identifier*>(callee.get())) {
        if (auto signature = tc.currentScope().getFunction(ident->name)) {
            if (arguments.size() < signature->size()) {
                auto it = arguments.begin();
                std::vector<Register> inferredArguments;
                std::vector<SynthesizedTypeExpression*> synthesizedTypes;
                for (uint32_t i = 0; i < signature->size(); i++) {
                    if (!signature->at(i)) {
                        ++it;
                        continue;
                    }

                    Register tmp = tc.generator().newLocal();
                    auto argument = std::make_unique<SynthesizedTypeExpression>(location);
                    inferredArguments.emplace_back(tmp);
                    synthesizedTypes.emplace_back(argument.get());
                    it = ++arguments.emplace(it, std::move(argument));
                }
                ASSERT(it == arguments.end(), "OOPS");
                ASSERT(inferredArguments.size(), "OOPS");
                ASSERT(inferredArguments.size() == synthesizedTypes.size(), "OOPS");
                tc.generator().inferImplicitParameters(calleeType, inferredArguments);

                for (unsigned i = 0; i < inferredArguments.size(); i++)
                    synthesizedTypes[i]->typeIndex = tc.generator().storeConstant(inferredArguments[i]);
            }
        }
    }
}

void CallExpression::infer(TypeChecker& tc, Register result)
{
    TypeChecker::Scope scope(tc);
    TypeChecker::UnificationScope unificationScope(tc);
    Label done = tc.generator().label();
    checkCallee(tc, result, done);
    checkArguments(tc, result, done);
    tc.generator().getField(result, result, TypeFunction::returnTypeField);
    unificationScope.resolve(result, result);
    tc.newValue(result, result);
    tc.generator().emit(done);
}

void SubscriptExpression::infer(TypeChecker& tc, Register result)
{
    TypeChecker::UnificationScope scope(tc);

    index->check(tc, tc.numberType());

    Register tmp = tc.generator().newLocal();
    tc.generator().newVarType(tmp, "T", /* inferred */ true, /* rigid */ false, tc.typeType());
    tc.generator().newArrayType(result, tmp);
    target->check(tc, result);
    scope.resolve(result, tmp);
    tc.generator().newValue(result, result);
}

void MemberExpression::infer(TypeChecker& tc, Register result)
{
    TypeChecker::UnificationScope scope(tc);
    Register tmp = tc.generator().newLocal();
    tc.generator().newVarType(tmp, "T", /* inferred */ true, /* rigid */ false, tc.typeType());
    tc.generator().newRecordType(result, { { property->name, tmp } });
    object->check(tc, result);
    scope.resolve(result, tmp);
    tc.generator().newValue(result, result);
}

void ObjectLiteralExpression::infer(TypeChecker& tc, Register result)
{
    generateForTypeChecking(tc, result);
}

void ArrayLiteralExpression::infer(TypeChecker& tc, Register result)
{
    generateForTypeChecking(tc, result);
}

void LiteralExpression::infer(TypeChecker& tc, Register result)
{
    literal->generate(tc.generator(), result);
}

// Types
void TypedIdentifier::infer(TypeChecker& tc, Register result)
{
    if (isSubtype || dynamic_cast<TypeTypeExpression*>(type.get())) {
        Register type = tc.typeType();
        if (isSubtype) {
            type = result;
            tc.inferAsType(this->type, result);
        }
        tc.generator().newVarType(result, name->name, inferred, /* rigid */ true, type);
        tc.generator().newBindingType(result, name->name, result);
        tc.insert(name->name, result);
        return;
    }

    if (inferred)
        tc.generator().typeError(location, "Only type arguments can be inferred");

    tc.inferAsType(type, result);
    Register tmp = tc.generator().newLocal();
    tc.generator().newBindingType(result, name->name, result);
    tc.generator().newValue(tmp, result);
    tc.insert(name->name, tmp);
}

void TypeTypeExpression::infer(TypeChecker& tc, Register result)
{
    tc.generator().move(result, tc.typeType());
}

void SynthesizedTypeExpression::infer(TypeChecker& tc, Register)
{
    tc.generator().typeError(location, "Should not infer type for SynthesizedTypeExpression");
}

void ArrayTypeExpression::infer(TypeChecker& tc, Register result)
{
    itemType->check(tc, tc.typeType());
    generateForTypeChecking(tc, result);
}

void ObjectTypeExpression::infer(TypeChecker& tc, Register result)
{
    for (auto& field : fields)
        field.second->check(tc, tc.typeType());
    generateForTypeChecking(tc, result);
}

void TupleTypeExpression::infer(TypeChecker& tc, Register result)
{
    for (auto& item : items)
        item->check(tc, tc.typeType());
    generateForTypeChecking(tc, result);
}

void FunctionTypeExpression::infer(TypeChecker& tc, Register result)
{
    for (auto& param : parameters)
        param->check(tc, tc.typeType());
    returnType->check(tc, tc.typeType());
    generateForTypeChecking(tc, result);
}

void UnionTypeExpression::infer(TypeChecker& tc, Register result)
{
    lhs->check(tc, tc.typeType());
    rhs->check(tc, tc.typeType());
    generateForTypeChecking(tc, result);
}

// Patterns

void IdentifierPattern::infer(TypeChecker& tc, Register result)
{
    tc.generator().newVarType(result, "T", /* inferred */ true, /* rigid */ false, tc.typeType());
    tc.generator().newValue(result, result);
    tc.insert(name->name, result);
}

void ObjectPattern::infer(TypeChecker& tc, Register result)
{
    Register tmp = tc.generator().newLocal();
    tc.generator().newRecordType(result, { });
    for (auto& entry : entries) {
        entry.second->infer(tc, tmp);
        tc.generator().getTypeForValue(tmp, tmp);
        tc.generator().setField(result, entry.first->name, tmp);
    }
    tc.generator().newValue(result, result);
}

void UnderscorePattern::infer(TypeChecker& tc, Register result)
{
    tc.topValue(result);
}

void LiteralPattern::infer(TypeChecker& tc, Register result)
{
    literal->generate(tc.generator(), result);
}
