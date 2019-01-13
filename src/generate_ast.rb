class ASTNode < Struct.new(:name, :parent, :fields, :extra_methods)
    def cpp_struct
        <<-EOS
        struct #{name} : public #{parent} {
            using #{parent}::#{parent};

            #{constructor}

            #{properties}

            #{dump}

            #{(extra_methods || []).map { |m| "#{m};"}.join("\n")}
        };
        EOS
    end

    def properties
        return unless fields

        fields.map do |name, type|
            "#{type} #{name};"
        end.join("\n")
    end

    def constructor
        <<-EOS
        #{name}(SourceLocation loc)
            : #{parent}(loc)
        {
        }
        EOS
    end

    def dump
        <<-EOS
        void dump(std::ostream& out, unsigned indentation) const
        {
            dumpStart(out, indentation, "#{name}");
            #{dumpFields}
            dumpEnd(out, indentation);
        }
        EOS
    end

    def dumpFields
        return unless fields

        fields.map do |name,_|
            "dumpField(out, indentation + 1, \"#{name}\", #{name});"
        end.join("\n")
    end
end

$ast_nodes = []
$includes = []
$declarations = []

def import(name)
    if name.is_a? String
        $includes << "#include \"#{name}\""
    else
        $includes << "#include <#{name.to_s}>"
    end
end

def declare(type)
    $declarations << type
end

def ast_node(name, meta = {})
    $ast_nodes << ASTNode.new(name[0], name[1], meta[:fields], meta[:extra_methods])
end

class Symbol
    def <(other_symbol)
        [self, other_symbol]
    end
end

$context = binding

def load_definitions(file)
    $context.eval(File.read(file), file)
end

def generate_file(file)
    File.write file, <<-EOS
    #pragma once

    #include "Node.h"
    #include "Register.h"

    class BytecodeGenerator;
    #{$includes.join("\n")}

    #{$declarations.map { |t| "struct #{t.to_s};" }.join("\n")}

    #{$ast_nodes.map(&:cpp_struct).join("\n\n")}
    EOS
end

load_definitions(ARGV[0])
generate_file(ARGV[1])
