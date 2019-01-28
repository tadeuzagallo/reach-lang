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
    TypeChecker tc(vm);
    const Type* type = tc.check(program);
    if (!type) {
        tc.reportErrors(std::cerr);
        return EXIT_FAILURE;
    }

    auto bytecode = program->generate(vm);
    vm.globalBlock = bytecode.get();
    Interpreter interpreter { vm, *bytecode, &vm.globalEnvironment };
    auto result = interpreter.run();
    std::cout << "End: " << result << " : " << *type << std::endl;

    return EXIT_SUCCESS;
}
