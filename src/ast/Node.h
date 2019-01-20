#pragma once
#include "SourceLocation.h"
#include "Token.h"
#include "Assert.h"

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>
#include <stdlib.h>

class Type;
class TypeChecker;

static std::string indent(unsigned size)
{
  return std::string(size * 2, ' ');
}

template <typename T>
void dumpField(std::ostream& out, unsigned indentation, std::string name, const T& value);

template <typename T>
void dumpValue(std::ostream& out, unsigned indentation, const std::unique_ptr<T>& value)
{
  value->dump(out, indentation);
}

template<typename T>
void dumpValue(std::ostream& out, unsigned indentation, const std::optional<T>& value)
{
  if (value)
    dumpValue(out, indentation, *value);
  else
    out << "nullopt";
}

template<typename T>
void dumpValue(std::ostream& out, unsigned indentation, const std::vector<T>& value)
{
  out << "[" << std::endl;
  for (auto& v : value) {
    out << indent(indentation + 1);
    dumpValue(out, indentation + 1, v);
    out << "," << std::endl;
  }
  out << indent(indentation) << "]";
}

template<typename K, typename V>
void dumpValue(std::ostream& out, unsigned indentation, const std::map<K, V>& value)
{
  out << "{" << std::endl;
  for (const auto& it : value) {
    out << indent(indentation + 1);
    dumpValue(out, indentation + 1, it.first);
    out << " = ";
    dumpValue(out, 0, it.second);
    out << "," << std::endl;
  }
  out << indent(indentation) << "}";
}

template <typename... T>
void dumpValue(std::ostream& out, unsigned indentation, const std::variant<T...>& value)
{
  std::visit([&](auto&& value) {
    dumpValue(out, indentation, value);
  }, value);
}

template<typename T>
void dumpValue(std::ostream& out, unsigned, T value)
{
  out << value;
}

template <typename T>
void dumpField(std::ostream& out, unsigned indentation, std::string name, const T& value)
{
  out << indent(indentation) << name << ": ";
  dumpValue(out, indentation, value);
  out << "," << std::endl;
}

struct Node {
  Node(const Token& token)
    : location(token.location)
  { }

  virtual ~Node() = default;

  virtual void dump(std::ostream&, unsigned = 0) const = 0;
  virtual void dumpStart(std::ostream& out, unsigned indentation, std::string&& name) const
  {
    out << name << " {" << std::endl;

    if (!std::getenv("PRINT_AST_LOCATIONS"))
      return;

    auto dumpPosition = [&, indentation = indentation + 2](std::string name, const SourcePosition& pos) {
      out << indent(indentation) << name << "; SourcePosition {" << std::endl;
      dumpField(out, indentation + 1, "line", pos.line);
      dumpField(out, indentation + 1, "column", pos.column);
      dumpField(out, indentation + 1, "offset", pos.offset);
      out << indent(indentation) << "}," << std::endl;
    };

    out << indent(indentation + 1) << "location: SourceLocation {" << std::endl;
    dumpField(out, indentation + 2, "filename", location.file.name);
    dumpPosition("start", location.start);
    dumpPosition("end", location.end);
    out << indent(indentation + 1) << "}," << std::endl;

  }

  void dumpEnd(std::ostream& out, unsigned indentation) const
  {
    out << indent(indentation) << "}";
  }

  SourceLocation location;

protected:
  Node(SourceLocation& loc)
    : location(loc)
  { }
};
