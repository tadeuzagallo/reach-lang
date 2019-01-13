CXX := clang++
LDFLAGS := -Wl,-no_pie
CXXFLAGS := -std=c++17 -g

SRCS := $(shell find src -name '*.cpp')
HEADERS := $(shell find src -name '*.h')
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))
INCLUDE_DIRS=$(patsubst %, -I %, $(shell find src -type 'd') + $(shell find build -type 'd'))

RUBY_AST := $(shell find 'src/ast' -name '*.rb')
GENERATED_SOURCES = build/bytecode/Instructions.h
GENERATED_SOURCES += $(patsubst src/%.rb,build/%.h,$(RUBY_AST))

build/reach: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

build/%.o: src/%.cpp $(HEADERS) $(GENERATED_SOURCES)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_DIRS)

build/ast/%.h: src/ast/%.rb src/generate_ast.rb
	@mkdir -p $(@D)
	ruby src/generate_ast.rb $< $@

build/bytecode/Instructions%h buid/bytecode/InstructionMacros%h: src/bytecode/instructions.rb src/generate_instructions.rb
	@mkdir -p $(@D)
	ruby src/generate_instructions.rb $< build/bytecode/Instructions.h build/bytecode/InstructionMacros.h

clean:
	rm -rf build

.PRECIOUS: $(GENERATED_SOURCES)
