#include "HoleCodegen.h"

#include "Array.h"
#include "BytecodeGenerator.h"
#include "Hole.h"
#include "Object.h"
#include "Tuple.h"

template<typename T>
void holeCodegen(T, BytecodeGenerator&, Register);

template<>
void holeCodegen(Value, BytecodeGenerator&, Register);

BytecodeBlock* holeCodegen(Value hole, BytecodeGenerator& generator)
{
    Register tmp = generator.newLocal();
    holeCodegen(hole, generator, tmp);
    return generator.finalize(tmp);
}

template<>
void holeCodegen(Array* array, BytecodeGenerator& generator, Register dst)
{
    generator.loadConstant(dst, Value::unit());
    generator.newArray(dst, dst, array->size());
    uint32_t i = 0;
    for (const auto& item : *array) {
        Register tmp = generator.newLocal();
        holeCodegen(item, generator, tmp);
        generator.setArrayIndex(dst, i++, tmp);
    }
}

template<>
void holeCodegen(Object* object, BytecodeGenerator& generator, Register dst)
{
    // TODO: add concept of structures
    generator.loadConstant(dst, Value::unit());
    generator.newObject(dst, dst, object->size());
    Register tmp = generator.newLocal();
    for (const auto& field : *object) {
        holeCodegen(field.second, generator, tmp);
        generator.setField(dst, field.first, tmp);
    }
}

template<>
void holeCodegen(Tuple* tuple, BytecodeGenerator& generator, Register dst)
{
    generator.loadConstant(dst, Value::unit());
    generator.newTuple(dst, dst, tuple->size());
    uint32_t i = 0;
    for (const auto& item : *tuple) {
        Register tmp = generator.newLocal();
        holeCodegen(item, generator, tmp);
        generator.setTupleIndex(dst, i++, tmp);
    }
}

template<>
void holeCodegen(Hole* hole, BytecodeGenerator& generator, Register dst)
{
    hole->generate(generator, dst);
}

template<>
void holeCodegen(Type* type, BytecodeGenerator& generator, Register dst)
{
    type->generate(generator, dst);
}

template<>
void holeCodegen(Value value, BytecodeGenerator& generator, Register dst)
{
    if (!value.isCell() && !value.hasHole()) {
        generator.loadConstant(dst, value);
        return;
    }

    Cell* cell = value.asCell();
    if (cell->is<Array>()) {
        holeCodegen(cell->cast<Array>(), generator, dst);
        return;
    }

    if (cell->is<Tuple>()) {
        holeCodegen(cell->cast<Tuple>(), generator, dst);
        return;
    }

    if (cell->is<Hole>()) {
        holeCodegen(cell->cast<Hole>(), generator, dst);
        return;
    }

    if (cell->is<Type>()) {
        holeCodegen(cell->cast<Type>(), generator, dst);
        return;
    }

    if (cell->is<Object>()) {
        holeCodegen(cell->cast<Object>(), generator, dst);
        return;
    }

    generator.loadConstant(dst, value);
}

void HoleVariable::generate(BytecodeGenerator& generator, Register dst) const
{
    generator.getLocal(dst, name()->str());
}

void HoleCall::generate(BytecodeGenerator& generator, Register dst) const
{
    Register calleeReg = generator.newLocal();
    holeCodegen(callee(), generator, calleeReg);
    std::vector<Register> args;
    Array* arguments = this->arguments();
    for (size_t i = 0; i < arguments->size(); i++)
        args.push_back(generator.newLocal());
    for (size_t i = 0; i < arguments->size(); i++)
        holeCodegen(arguments->getIndex(i), generator, args[i]);
    generator.call(dst, calleeReg, args);
}

void HoleSubscript::generate(BytecodeGenerator& generator, Register dst) const
{
    holeCodegen(target(), generator, dst);
    Register tmp = generator.newLocal();
    holeCodegen(index(), generator, tmp);
    generator.getArrayIndex(dst, dst, tmp);
}

void HoleMember::generate(BytecodeGenerator& generator, Register dst) const
{
    holeCodegen(object(), generator, dst);
    generator.getField(dst, dst, property()->str());
}

// Types

void Type::generate(BytecodeGenerator& generator, Register dst) const
{
    generator.loadConstant(dst, this);
}

void TypeFunction::generate(BytecodeGenerator& generator, Register dst) const
{
    size_t size = params()->size();
    std::vector<Register> params;
    for (uint32_t i = 0; i < size; i++)
        params.emplace_back(generator.newLocal());
    for (uint32_t i = 0; i < size; i++)
        holeCodegen(this->params()->getIndex(i), generator, params[size - i - 1]);
    holeCodegen(returnType(), generator, dst);
    generator.newFunctionType(dst, params, dst, m_inferredParameters);
}

void TypeArray::generate(BytecodeGenerator& generator, Register dst) const
{
    holeCodegen(itemType(), generator, dst);
    generator.newArrayType(dst, dst);
}

void TypeTuple::generate(BytecodeGenerator& generator, Register dst) const
{
    generator.newTupleType(dst, itemsTypes()->size());
    Register itemsTypes = generator.newLocal();
    generator.getField(itemsTypes, dst, TypeTuple::itemsTypesField);
    Register tmp = generator.newLocal();
    for (uint32_t i = 0; i < this->itemsTypes()->size(); i++) {
        holeCodegen(this->itemsTypes()->getIndex(i), generator, tmp);
        generator.setArrayIndex(itemsTypes, i, tmp);
    }
}

void TypeRecord::generate(BytecodeGenerator& generator, Register dst) const
{
    generator.newRecordType(dst, {});
    Register tmp = generator.newLocal();
    for (const auto& field : *this) {
        holeCodegen(field.second, generator, tmp);
        generator.setField(dst, field.first, tmp);
    }
}

void TypeUnion::generate(BytecodeGenerator& generator, Register dst) const
{
    Register tmp = generator.newLocal();
    holeCodegen(lhs(), generator, dst);
    holeCodegen(rhs(), generator, tmp);
    generator.newUnionType(dst, dst, tmp);
}

void TypeBinding::generate(BytecodeGenerator& generator, Register dst) const
{
    holeCodegen(type(), generator, dst);
    generator.newBindingType(dst, name()->str(), dst);
}
