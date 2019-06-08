#include "program.h"
#include "BytecodeGenerator.h"
#include "RhString.h"

std::unique_ptr<BytecodeBlock> Program::generate(BytecodeGenerator& generator) const
{
    generator.emitLocation(location);
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
    generator.emitLocation(location);

    //ASSERT(!isConst, "TODO");
    (*initializer)->generate(generator, result);
    generator.setLocal(*name, result);
}

void FunctionDeclaration::generateImpl(BytecodeGenerator& generator, Register result, Register type)
{
    generator.emitLocation(location);

    BytecodeGenerator functionGenerator { generator.vm(), name->name };
    for (unsigned i = 0; i < parameters.size(); i++)
        functionGenerator.setLocal(*parameters[i]->name, Register::forParameter(i));
    Register functionResult = functionGenerator.newLocal();
    body->generate(functionGenerator, functionResult);
    auto block = functionGenerator.finalize(functionResult);

    generator.newFunction(result, std::move(block), type);
    generator.setLocal(*name, result);
}

void FunctionDeclaration::generate(BytecodeGenerator& generator, Register result)
{
    generator.emitLocation(location);

    generator.move(result, *valueRegister);
}

void StatementDeclaration::generate(BytecodeGenerator& generator, Register result)
{
    generator.emitLocation(location);

    statement->generate(generator, result);
}

// Statements
void EmptyStatement::generate(BytecodeGenerator& generator, Register)
{
    generator.emitLocation(location);
}

void BlockStatement::generate(BytecodeGenerator& generator, Register result)
{
    generator.emitLocation(location);

    Register localResult = Register::invalid();
    for (const auto& decl : declarations) {
        localResult = generator.newLocal();
        decl->generate(generator, localResult);
    }
    if (localResult.isValid()) {
        generator.move(result, localResult);
    }
}

void ReturnStatement::generate(BytecodeGenerator& generator, Register)
{
    generator.emitLocation(location);
}

void IfStatement::generate(BytecodeGenerator& generator, Register out)
{
    generator.emitLocation(location);

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

void BreakStatement::generate(BytecodeGenerator& generator, Register)
{
    generator.emitLocation(location);
    ASSERT_NOT_REACHED();
}

void ContinueStatement::generate(BytecodeGenerator& generator, Register)
{
    generator.emitLocation(location);
    ASSERT_NOT_REACHED();
}

void WhileStatement::generate(BytecodeGenerator& generator, Register)
{
    generator.emitLocation(location);
    ASSERT_NOT_REACHED();
}

void ForStatement::generate(BytecodeGenerator& generator, Register)
{
    generator.emitLocation(location);
    ASSERT_NOT_REACHED();
}

void ExpressionStatement::generate(BytecodeGenerator& generator, Register result)
{
    generator.emitLocation(location);

    expression->generate(generator, result);
}

// Expressions
void Identifier::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    generator.getLocal(dst, *this);
}

void BinaryExpression::generate(BytecodeGenerator& generator, Register)
{
    generator.emitLocation(location);
}

void ParenthesizedExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    expression->generate(generator, dst);
}

void ObjectLiteralExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    // TODO: add concept of structures
    generator.newObject(dst, fields.size());
    for (const auto& field : fields) {
        Register tmp = generator.newLocal();
        field.second->generate(generator, tmp);
        generator.setField(dst, field.first->name, tmp);
    }
}

void ArrayLiteralExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    generator.newArray(dst, items.size());
    for (unsigned i = 0; i < items.size(); i++) {
        Register tmp = generator.newLocal();
        items[i]->generate(generator, tmp);
        generator.setArrayIndex(dst, i, tmp);
    }
}

void CallExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

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
    generator.emitLocation(location);

    target->generate(generator, dst);
    Register tmp = generator.newLocal();
    index->generate(generator, tmp);
    generator.getArrayIndex(dst, dst, tmp);
}

void MemberExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    object->generate(generator, dst);
    generator.getField(dst, dst, property->name);
}

void LiteralExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    return literal->generate(generator, dst);
}

void SynthesizedTypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    return generator.loadGlobalConstant(dst, *typeIndex);
}

void TypeExpression::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    type->generate(generator, dst);
}


// Literals

void BooleanLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    return generator.loadConstant(dst, value);
}

void NumericLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    return generator.loadConstant(dst, value);
}

void StringLiteral::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    String* string = String::create(generator.vm(), value);
    return generator.loadConstant(dst, string);
}


// Types

void ASTTypeType::generate(BytecodeGenerator& generator, Register dst)
{
    generator.emitLocation(location);

    generator.loadConstant(dst, generator.vm().typeType);
}
