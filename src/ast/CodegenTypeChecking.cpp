#include "BytecodeGenerator.h"
#include "RhString.h"
#include "TypeChecker.h"
#include "TypeExpressions.h"

void Identifier::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    tc.currentScope().lookup(dst, name);
}

void ParenthesizedExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    expression->generateForTypeChecking(tc, dst);
}

void LazyExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    expression->generateForTypeChecking(tc, dst);
}

void ObjectLiteralExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    std::vector<std::pair<std::string, Register>> fieldRegisters;

    for (const auto& pair : fields)
        fieldRegisters.emplace_back(std::make_pair(pair.first->name, tc.generator().newLocal()));

    unsigned i = 0;
    for (auto& pair : fields) {
        Register type = fieldRegisters[i++].second;
        pair.second->infer(tc, type);
        tc.generator().getTypeForValue(type, type);
    }

    tc.newRecordType(dst, fieldRegisters);
    // TODO: add concept of structures
    tc.generator().newObject(dst, dst, fields.size());
    Register tmp = tc.generator().newLocal();
    for (const auto& field : fields) {
        field.second->infer(tc, tmp);
        //tc.generator().getTypeForValue(tmp, tmp);
        tc.generator().setField(dst, field.first->name, tmp);
    }
}

void ArrayLiteralExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    tc.generator().newArray(dst, dst, items.size());
    for (unsigned i = 0; i < items.size(); i++) {
        Register tmp = tc.generator().newLocal();
        items[i]->infer(tc, tmp);
        //tc.generator().getTypeForValue(tmp, tmp);
        tc.generator().setArrayIndex(dst, i, tmp);
    }
}

void TupleExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    tc.generator().newTuple(dst, dst, items.size());
    for (unsigned i = 0; i < items.size(); i++) {
        Register tmp = tc.generator().newLocal();
        items[i]->infer(tc, tmp);
        //tc.generator().getTypeForValue(tmp, tmp);
        tc.generator().setTupleIndex(dst, i, tmp);
    }
}

void CallExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    Register calleeReg = tc.generator().newLocal();
    callee->infer(tc, calleeReg);
    //tc.generator().getTypeForValue(calleeReg, calleeReg);
    std::vector<Register> args;
    for (size_t i = 0; i < arguments.size(); i++)
        args.push_back(tc.generator().newLocal());
    for (size_t i = 0; i < arguments.size(); i++)
        arguments[i]->generateForTypeChecking(tc, args[i]);
    tc.generator().newCallHole(dst, calleeReg, args);
}

void SubscriptExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    target->generateForTypeChecking(tc, dst);
    //tc.generator().getTypeForValue(dst, dst);
    Register tmp = tc.generator().newLocal();
    index->generateForTypeChecking(tc, tmp);
    tc.generator().newSubscriptHole(dst, dst, tmp);
}

void MemberExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    object->generateForTypeChecking(tc, dst);
    //tc.generator().getTypeForValue(dst, dst);
    tc.generator().newMemberHole(dst, dst, property->name);
}

void LiteralExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    return literal->generate(tc.generator(), dst);
}

// Type Expressions

void TypeTypeExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    generate(tc.generator(), dst);
}

void ObjectTypeExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    tc.generator().newRecordType(dst, {});
    Register tmp = tc.generator().newLocal();
    for (const auto& field : fields) {
        field.second->generateForTypeChecking(tc, tmp);
        //tc.generator().getTypeForValue(tmp, tmp);
        tc.generator().setField(dst, field.first->name, tmp);
    }
}

void TupleTypeExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    tc.generator().newTupleType(dst, items.size());
    Register itemsTypes = tc.generator().newLocal();
    tc.generator().getField(itemsTypes, dst, TypeTuple::itemsTypesField);
    Register tmp = tc.generator().newLocal();
    for (uint32_t i = 0; i < items.size(); i++) {
        items[i]->generateForTypeChecking(tc, tmp);
        //tc.generator().getTypeForValue(tmp, tmp);
        tc.generator().setArrayIndex(itemsTypes, i, tmp);
    }
}

void FunctionTypeExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    size_t size = parameters.size();
    std::vector<Register> params;
    for (uint32_t i = 0; i < size; i++)
        params.emplace_back(tc.generator().newLocal());
    for (uint32_t i = 0; i < size; i++) {
        parameters[i]->generateForTypeChecking(tc, params[size - i - 1]);
        //tc.generator().getTypeForValue(params[size - i - 1], params[size - i - 1]);
    }
    returnType->generateForTypeChecking(tc, dst);
    //tc.generator().getTypeForValue(dst, dst);
    tc.generator().newFunctionType(dst, params, dst, 0);
}

void UnionTypeExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    Register tmp = tc.generator().newLocal();
    lhs->generateForTypeChecking(tc, dst);
    //tc.generator().getTypeForValue(dst, dst);
    rhs->generateForTypeChecking(tc, tmp);
    //tc.generator().getTypeForValue(tmp, tmp);
    tc.generator().newUnionType(dst, dst, tmp);
}

void ArrayTypeExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    itemType->generateForTypeChecking(tc, dst);
    //tc.generator().getTypeForValue(dst, dst);
    tc.generator().newArrayType(dst, dst);
}

void SynthesizedTypeExpression::generateForTypeChecking(TypeChecker& tc, Register dst)
{
    generate(tc.generator(), dst);
}
