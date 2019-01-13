$context = binding

class Instruction < Struct.new(:name, :fields)
  def cpp_struct
    <<-EOS
    struct #{name} : public Instruction {
      static constexpr Instruction::ID ID = Instruction::#{name};
      #{properties}

      #{emit}

      #{dump}
    };
    EOS
  end

  def properties
    return unless fields

    fields.map do |name, type|
      "#{type} #{name};"
    end.join("\n")
  end

  def emit
      args = ["BytecodeGenerator* __generator"]
      args += fields.map { |name, type| "#{type} #{name}" }
      args = args.join(", ")

      <<-EOS
      template<typename BytecodeGenerator>
      static void emit(#{args})
      {
          __generator->emit(ID);
          #{fields.map { |name, _|  "__generator->emit(#{name});" }.join("\n")}
      }
      EOS
  end

  def dump
    <<-EOS
    void dump(std::ostream& out) const
    {
        unsigned idx = 0;
        #{fields.map { |name, _| "if (idx++) out << \", \"; out << \"#{name}: \" << #{name};" }.join("\n")}
    }
    EOS
  end
end

$instructions = []

def instruction(name, fields = {})
  $instructions << Instruction.new(name.to_s, fields)
end

def load_definitions(file)
  $context.eval(File.read(file), file)
end

def generate_instructions(file)
  File.write file, <<-EOS
  #pragma once

  #include "Instruction.h"
  #include "Register.h"

  #{$instructions.map(&:cpp_struct).join("\n\n")}
  EOS
end

def generate_macros(file)
  File.write file, <<-EOS
  #pragma once

  #{instruction_count}
  #{instruction_ids}
  #{instruction_sizes}
  #{instruction_names}
  #{for_each_instruction}
  EOS
end

def instruction_count
  <<-EOS
  #define INSTRUCTION_COUNT #{$instructions.size}
  EOS
end

def instruction_ids
  <<-EOS
  #define INSTRUCTION_IDS #{$instructions.map(&:name).join(", \\\n")}
  EOS
end

def instruction_names
  <<-EOS
  #define INSTRUCTION_NAMES #{$instructions.map {|i| '"' + i.name + '"'}.join(", \\\n")}
  EOS
end

def instruction_sizes
  <<-EOS
  #define INSTRUCTION_SIZES #{$instructions.map {|i| (i.fields.size + 1).to_s}.join(", \\\n")}
  EOS
end

def for_each_instruction
  <<-EOS
  #define FOR_EACH_INSTRUCTION(macro) \\
      #{$instructions.map { |i| "macro(#{i.name})" }.join("\\\n")}
  EOS
end

load_definitions(ARGV[0])
generate_instructions(ARGV[1])
generate_macros(ARGV[2])
