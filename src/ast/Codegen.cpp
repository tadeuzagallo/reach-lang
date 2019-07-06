#include "program.h"
#include "BytecodeGenerator.h"
#include "RhString.h"
#include "TypeExpressions.h"

std::unique_ptr<BytecodeBlock> Program::generate(BytecodeGenerator& generator) const
{
    Register result = generator.newLocal();
    for (const auto& decl : declarations)
        decl->generate(generator, result);
    if (!declarations.size())
        generator.loadConstant(result, Value::unit());
    return generator.finalize(result);
}

// Declarations
void LexicalDeclaration::generate(BytecodeGenerator& generator, Register result)
{
    initializer->generate(generator, result);
    generator.setLocal(*name, result);
}

void FunctionDeclaration::generateImpl(BytecodeGenerator& generator, Register result)
{
    for (unsigned i = 0; i < parameters.size(); i++)
        generator.setLocal(*parameters[i]->name, Register::forParameter(i));
    body->generate(generator, result);
}

void FunctionDeclaration::generate(BytecodeGenerator& generator, Register result)
{
    generator.newFunction(result, functionIndex);
    generator.setLocal(name->name, result);
}

void StatementDeclaration::generate(BytecodeGenerator& generator, Register result)
{
    statement->generate(generator, result);
}

// Statements
void EmptyStatement::generate(BytecodeGenerator&, Register)
{
}

void BlockStatement::generate(BytecodeGenerator& generator, Register result)
{
    for (const auto& decl : declarations)
        decl->generate(generator, result);
    if (!declarations.size())
        generator.loadConstant(result, Value::unit());
}

void ReturnStatement::generate(BytecodeGenerator&, Register)
{
    ASSERT_NOT_REACHED();
}

void IfStatement::generate(BytecodeGenerator& generator, Register out)
{
    Label alt = generator.label();
    Label end = generator.label();

    Register tmp = generator.newLocal();
    condition->generate(generator, tmp);
    generator.jumpIfFalse(tmp, alt);

    {
        consequent->generate(generator, out);
        generator.jump(end);
    }

    if (alternate) {
        generator.emit(alt);
        (*alternate)->generate(generator, out);
    }

    generator.emit(end);
}

void BreakStatement::generate(BytecodeGenerator&, Register)
{
    ASSERT_NOT_REACHED();
}

void ContinueStatement::generate(BytecodeGenerator&, Register)
{
    ASSERT_NOT_REACHED();
}

void WhileStatement::generate(BytecodeGenerator&, Register)
{
    ASSERT_NOT_REACHED();
}

void ForStatement::generate(BytecodeGenerator&, Register)
{
    ASSERT_NOT_REACHED();
}

void ExpressionStatement::generate(BytecodeGenerator& generator, Register result)
{
    expression->generate(generator, result);
}

// Expressions
void Identifier::generate(BytecodeGenerator& generator, Register dst)
{
    generator.getLocal(dst, *this);
}

void ParenthesizedExpression::generate(BytecodeGenerator& generator, Register dst)
{
    expression->generate(generator, dst);
}

void ObjectLiteralExpression::generate(BytecodeGenerator& generator, Register dst)
{
    // TODO: add concept of structures
    generator.newObject(dst, fields.size());
    Register tmp = generator.newLocal();
    for (const auto& field : fields) {
        field.second->generate(generator, tmp);
        generator.setField(dst, field.first->name, tmp);
    }
}

void ArrayLiteralExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.newArray(dst, items.size());
    for (unsigned i = 0; i < items.size(); i++) {
        Register tmp = generator.newLocal();
        items[i]->generate(generator, tmp);
        generator.setArrayIndex(dst, i, tmp);
    }
}

void TupleExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.newTuple(dst, items.size());
    for (unsigned i = 0; i < items.size(); i++) {
        Register tmp = generator.newLocal();
        items[i]->generate(generator, tmp);
        generator.setTupleIndex(dst, i, tmp);
    }
}

void CallExpression::generate(BytecodeGenerator& generator, Register dst)
{
    Register calleeReg = generator.newLocal();
    callee->generate(generator, calleeReg);
    std::vector<Register> args;
    for (size_t i = 0; i < arguments.size(); i++)
        args.push_back(generator.newLocal());
    for (size_t i = 0; i < arguments.size(); i++)
        arguments[i]->generate(generator, args[i]);
    generator.call(dst, calleeReg, args);
}

void SubscriptExpression::generate(BytecodeGenerator& generator, Register dst)
{
    target->generate(generator, dst);
    Register tmp = generator.newLocal();
    index->generate(generator, tmp);
    generator.getArrayIndex(dst, dst, tmp);
}

void MemberExpression::generate(BytecodeGenerator& generator, Register dst)
{
    object->generate(generator, dst);
    generator.getField(dst, dst, property->name);
}

void LiteralExpression::generate(BytecodeGenerator& generator, Register dst)
{
    return literal->generate(generator, dst);
}

// Literals

void BooleanLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    return generator.loadConstant(dst, value);
}

void NumericLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    return generator.loadConstant(dst, value);
}

void StringLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    String* string = String::create(generator.vm(), value);
    return generator.loadConstant(dst, string);
}


// Types

void TypeTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.loadConstant(dst, generator.vm().typeType);
}

void ObjectTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.newRecordType(dst, {});
    Register fieldsRegister = generator.newLocal();
    Register tmp = generator.newLocal();
    generator.getField(fieldsRegister, dst, TypeRecord::fieldsField);
    for (const auto& field : fields) {
        field.second->generate(generator, tmp);
        generator.setField(fieldsRegister, field.first->name, tmp);
    }
}

void TupleTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.newTupleType(dst, items.size());
    Register itemsTypes = generator.newLocal();
    generator.getField(itemsTypes, dst, TypeTuple::itemsTypesField);
    Register tmp = generator.newLocal();
    for (uint32_t i = 0; i < items.size(); i++) {
        items[i]->generate(generator, tmp);
        generator.setArrayIndex(itemsTypes, i, tmp);
    }
}

void FunctionTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    size_t size = parameters.size();
    std::vector<Register> params;
    for (uint32_t i = 0; i < size; i++)
        params.emplace_back(generator.newLocal());
    for (uint32_t i = 0; i < size; i++)
        parameters[i]->generate(generator, params[size - i - 1]);
    returnType->generate(generator, dst);
    generator.newFunctionType(dst, params, dst, 0);
}

void UnionTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    Register tmp = generator.newLocal();
    lhs->generate(generator, dst);
    rhs->generate(generator, tmp);
    generator.newUnionType(dst, dst, tmp);
}

void ArrayTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    itemType->generate(generator, dst);
    generator.newArrayType(dst, dst);
}

void SynthesizedTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    return generator.loadConstantIndex(dst, typeIndex);
}
