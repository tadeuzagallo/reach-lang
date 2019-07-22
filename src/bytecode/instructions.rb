import "Instruction.h"
import "Register.h"
import "Type.h"

instruction :Enter

instruction :End,
    dst: :Register

instruction :Move,
    dst: :Register,
    src: :Register

instruction :LoadConstant,
    dst: :Register,
    constantIndex: :uint32_t

instruction :StoreConstant,
    constantIndex: :uint32_t,
    value: :Register

instruction :GetLocal,
    dst: :Register,
    identifierIndex: :uint32_t

instruction :SetLocal,
    identifierIndex: :uint32_t,
    src: :Register

instruction :NewArray,
    dst: :Register,
    type: :Register,
    initialSize: :uint32_t

instruction :SetArrayIndex,
    src: :Register,
    index: :uint32_t,
    value: :Register

instruction :GetArrayIndex,
    dst: :Register,
    array: :Register,
    index: :Register

instruction :GetArrayLength,
    dst: :Register,
    array: :Register

instruction :NewTuple,
    dst: :Register,
    type: :Register,
    initialSize: :uint32_t

instruction :SetTupleIndex,
    tuple: :Register,
    index: :uint32_t,
    value: :Register

instruction :GetTupleIndex,
    dst: :Register,
    tuple: :Register,
    index: :Register

instruction :NewFunction,
    dst: :Register,
    functionIndex: :uint32_t

instruction :Call,
    dst: :Register,
    callee: :Register,
    argc: :uint32_t,
    firstArg: :Register

instruction :NewObject,
    dst: :Register,
    type: :Register,
    inlineSize: :uint32_t

instruction :SetField,
    object: :Register,
    fieldIndex: :uint32_t,
    value: :Register

instruction :GetField,
    dst: :Register,
    object: :Register,
    fieldIndex: :uint32_t

instruction :TryGetField,
    dst: :Register,
    object: :Register,
    fieldIndex: :uint32_t,
    target: :int32_t

instruction :Jump,
    target: :int32_t

instruction :JumpIfFalse,
    condition: :Register,
    target: :int32_t

instruction :IsEqual,
    dst: :Register,
    lhs: :Register,
    rhs: :Register

instruction :RuntimeError,
    messageIndex: :uint32_t

instruction :IsCell,
    dst: :Register,
    value: :Register,
    kind: "Cell::Kind"

# Type checking instructions
instruction :PushScope

instruction :PopScope

instruction :PushUnificationScope

instruction :PopUnificationScope

instruction :Unify,
    lhs: :Register,
    rhs: :Register

instruction :ResolveType,
    dst: :Register,
    type: :Register

instruction :CheckType,
    dst: :Register,
    type: :Register,
    expected: "Type::Class"

instruction :CheckTypeOf,
    dst: :Register,
    type: :Register,
    expected: "Type::Class"

instruction :TypeError,
    messageIndex: :uint32_t

instruction :InferImplicitParameters,
    function: :Register,
    parameterCount: :uint32_t,
    firstParameter: :Register

# Create new types
instruction :NewVarType,
   dst: :Register,
   nameIndex: :uint32_t,
   isInferred: :uint32_t,
   isRigid: :uint32_t

instruction :NewNameType,
    dst: :Register,
    nameIndex: :uint32_t

instruction :NewArrayType,
    dst: :Register,
    itemType: :Register

instruction :NewTupleType,
    dst: :Register,
    itemCount: :uint32_t

instruction :NewRecordType,
    dst: :Register,
    fieldCount: :uint32_t,
    firstKey: :Register,
    firstType: :Register

instruction :NewFunctionType,
    dst: :Register,
    paramCount: :uint32_t,
    firstParam: :Register,
    returnType: :Register,
    inferredParameters: :uint32_t

instruction :NewUnionType,
    dst: :Register,
    lhs: :Register,
    rhs: :Register

instruction :NewBindingType,
    dst: :Register,
    nameIndex: :uint32_t,
    type: :Register

# Holes

instruction :NewCallHole,
    dst: :Register,
    callee: :Register,
    argc: :uint32_t,
    firstArg: :Register

instruction :NewSubscriptHole,
    dst: :Register,
    target: :Register,
    index: :Register

instruction :NewMemberHole,
    dst: :Register,
    object: :Register,
    fieldIndex: :uint32_t

# Create new values from types
instruction :NewValue,
    dst: :Register,
    type: :Register

instruction :GetTypeForValue,
    dst: :Register,
    value: :Register
