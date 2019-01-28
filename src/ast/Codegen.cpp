#include "program.h"
#include "BytecodeGenerator.h"
#include "RhString.h"

std::unique_ptr<BytecodeBlock> Program::generate(VM& vm) const
{
    BytecodeGenerator generator { vm, "<global>" };
    Register result = Register::invalid();
    for (const auto& decl : declarations) {
        result = generator.newLocal();
        decl->generate(generator, result);
    }
    if (!result.isValid())
        result =  generator.newLocal();
    return generator.finalize(result);
}

// Declarations
void LexicalDeclaration::generate(BytecodeGenerator& generator, Register result)
{
    //ASSERT(!isConst, "TODO");
    (*initializer)->generate(generator, result);
    generator.setLocal(*name, result);
}

void FunctionDeclaration::generate(BytecodeGenerator& generator, Register result)
{
    BytecodeGenerator functionGenerator { generator.vm(), name->name };
    for (unsigned i = 0; i < parameters.size(); i++)
        functionGenerator.setLocal(*parameters[i]->name, Register::forParameter(i));
    Register functionResult = functionGenerator.newLocal();
    body->generate(functionGenerator, functionResult);
    auto block = functionGenerator.finalize(functionResult);

    generator.newFunction(result, std::move(block));
    generator.setLocal(*name, result);
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
    Register localResult = Register::invalid();
    for (const auto& decl : declarations) {
        localResult = generator.newLocal();
        decl->generate(generator, localResult);
    }
    if (localResult.isValid()) {
        generator.move(result, localResult);
    }
}

void ReturnStatement::generate(BytecodeGenerator&, Register)
{
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
}

void ContinueStatement::generate(BytecodeGenerator&, Register)
{
}

void WhileStatement::generate(BytecodeGenerator&, Register)
{
}

void ForStatement::generate(BytecodeGenerator&, Register)
{
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

void BinaryExpression::generate(BytecodeGenerator&, Register)
{
}

void ParenthesizedExpression::generate(BytecodeGenerator& generator, Register dst)
{
    expression->generate(generator, dst);
}

void ObjectLiteralExpression::generate(BytecodeGenerator& generator, Register dst)
{
    // TODO: add concept of structures
    generator.newObject(dst, fields.size());
    for (const auto& field : fields) {
        Register tmp = generator.newLocal();
        field.second->generate(generator, tmp);
        generator.setField(dst, *field.first, tmp);
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
    generator.getField(dst, dst, *property);
}

void LiteralExpression::generate(BytecodeGenerator& generator, Register dst)
{
    return literal->generate(generator, dst);
}

void SynthesizedTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    return generator.loadConstant(dst, binding->value());
}


// Literals

void BooleanLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    return generator.loadConstant(dst, Value { value });
}

void NumericLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    return generator.loadConstant(dst, Value { value });
}

void StringLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    String* string = String::create(generator.vm(), value.c_str(), value.length());
    return generator.loadConstant(dst, Value { string });
}
