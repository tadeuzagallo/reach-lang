#include "Assert.h"
#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include "Type.h"
#include "TypeChecker.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char** argv)
{
    ASSERT(argc == 2, "Expected target file as the only argument");

    const char* filename = argv[1];
    FILE* file = fopen(filename, "r");
    ASSERT(file, "Cannot open target file: %s", filename);

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source = reinterpret_cast<char*>(malloc(length));
    fread(source, length, 1, file);
    fclose(file);

    SourceFile sourceFile { filename, source, length };
    Lexer lexer { sourceFile };
    Parser parser { lexer };
    auto program = parser.parse();
    if (!program) {
        parser.reportErrors(std::cerr);
        return EXIT_FAILURE;
    }

    VM vm;
    BytecodeGenerator generator(vm);
    Register typeReg = Register::invalid();
    {
        TypeChecker tc(generator);
        typeReg = tc.check(program);
    }

    auto bytecode = program->generate(generator);
    vm.globalBlock = bytecode.get();
    Interpreter interpreter { vm, *bytecode, vm.globalEnvironment };
    Value type;
    Value result = interpreter.run({}, [&]{
        type = interpreter.reg(typeReg);
    });
    std::cout << "End: " << result << " : " << *type.type(vm) << std::endl;

    return EXIT_SUCCESS;
}
