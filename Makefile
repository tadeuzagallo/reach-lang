CXX := clang++
LDFLAGS := -Wl,-no_pie
CXXFLAGS := -std=c++17 -g

SRCS := $(shell find src -name '*.cpp')
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))
DEPS := $(patsubst src/%.cpp,build/%.d,$(SRCS))
INCLUDE_DIRS=$(patsubst %, -I %, $(shell find src -type 'd') + $(shell find build -type 'd'))

RUBY_AST := $(shell find 'src/ast' -name '*.rb')
GENERATED_SOURCES = build/bytecode/Instructions.h
GENERATED_SOURCES += $(patsubst src/%.rb,build/%.h,$(RUBY_AST))

build/reach: $(GENERATED_SOURCES) $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

build/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MF build/$*.d $(INCLUDE_DIRS)

build/ast/%.h: src/ast/%.rb src/generate_ast.rb
	@mkdir -p $(@D)
	ruby src/generate_ast.rb $< $@

build/bytecode/Instructions%h buid/bytecode/InstructionMacros%h: src/bytecode/instructions.rb src/generate_instructions.rb
	@mkdir -p $(@D)
	ruby src/generate_instructions.rb $< build/bytecode/Instructions.h build/bytecode/InstructionMacros.h

test:
	lit -v tests
	JIT_THRESHOLD=0 lit -v tests

clean:
	rm -rf build

-include $(DEPS)

.PRECIOUS: $(GENERATED_SOURCES)
