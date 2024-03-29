#pragma once

#include "Typed.h"
#include "VM.h"
#include <optional>
#include <unordered_map>

#define FIELD_NAME(__name) \
    static constexpr const char* __name##Field  = #__name; \

#define FIELD_CELL_GETTER(__type, __name) \
    __type* __name() const \
    { \
        return get(__name##Field).asCell<__type>(); \
    } \

#define FIELD_CELL_SETTER(__type, __name) \
    void set_##__name(__type* __value) \
    { \
        set(__name##Field, __value); \
    } \

#define FIELD_VALUE_GETTER(__type, __name, ...) \
    __type __name() const \
    { \
        return get(__name##Field) __VA_ARGS__; \
    } \

#define FIELD_VALUE_SETTER(__type, __name) \
    void set_##__name(__type __value) \
    { \
        set(__name##Field, __value); \
    } \

#define CELL_FIELD(__type, __name) \
    FIELD_NAME(__name) \
    FIELD_CELL_GETTER(__type, __name) \
    FIELD_CELL_SETTER(__type, __name) \

#define VALUE_FIELD(__type, __name, ...) \
    FIELD_NAME(__name) \
    FIELD_VALUE_GETTER(__type, __name, __VA_ARGS__) \
    FIELD_VALUE_SETTER(__type, __name) \

class Object : public Typed {
public:
    CELL(Object)

    void set(const std::string& field, Value value)
    {
        m_fields[field] = value;
    }

    Value get(const std::string& field) const
    {
        auto it = m_fields.find(field);
        ASSERT(it != m_fields.end(), "Unknown field: %s", field.c_str());
        return it->second;
    }

    std::optional<Value> tryGet(const std::string& field) const
    {
        auto it = m_fields.find(field);
        if (it == m_fields.end())
            return std::nullopt;
        return { it->second };
    }

    size_t size() const { return m_fields.size(); }
    std::unordered_map<std::string, Value>::iterator begin() { return m_fields.begin(); }
    std::unordered_map<std::string, Value>::iterator end() { return m_fields.end(); }
    std::unordered_map<std::string, Value>::const_iterator begin() const { return m_fields.begin(); }
    std::unordered_map<std::string, Value>::const_iterator end() const { return m_fields.end(); }

    bool operator==(const Object&) const;
    Object* substitute(VM&, const Substitutions&) const;

    void dump(std::ostream& out) const override;

protected:
    Object(Type* type, uint32_t inlineSize)
        : Typed(type)
    {
        (void)inlineSize; // TODO
    }

    template<typename T>
    Object(Type* type, const std::unordered_map<std::string, T>& fields)
        : Typed(type)
        , m_fields(fields.begin(), fields.end())
    {
    }

    Object(Type*, const BytecodeBlock&, uint32_t, const Value*, const Value*);

    void visit(const Visitor&) const override;

private:
    std::unordered_map<std::string, Value> m_fields;
};

// JIT helpers
extern "C" {
Object* createObject(VM&, Type*, uint32_t);
int64_t tryGetJIT(Object*, const std::string&);
}
