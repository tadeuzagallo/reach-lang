instruction :Enter

instruction :End,
    dst: :Register

instruction :Move,
    dst: :Register,
    src: :Register

instruction :LoadConstant,
    dst: :Register,
    constantIndex: :uint32_t

instruction :GetLocal,
    dst: :Register,
    identifierIndex: :uint32_t

instruction :SetLocal,
    identifierIndex: :uint32_t,
    src: :Register

instruction :NewArray,
    dst: :Register,
    initialSize: :uint32_t

instruction :SetArrayIndex,
    src: :Register,
    index: :uint32_t,
    value: :Register

instruction :GetArrayIndex,
    dst: :Register,
    array: :Register,
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
    inlineSize: :uint32_t

instruction :SetField,
    object: :Register,
    fieldIndex: :uint32_t,
    value: :Register

instruction :GetField,
    dst: :Register,
    object: :Register,
    fieldIndex: :uint32_t

instruction :Jump,
    target: :int32_t

instruction :JumpIfFalse,
    condition: :Register,
    target: :int32_t
