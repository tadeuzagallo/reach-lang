#pragma once

#include <iostream>

#define CELL(__type) \
    static constexpr Cell::Type type() { return Cell::Type::__type; } \
    \
    template<typename... Args> \
    static __type* create(VM& vm, Args&&... args) \
    { \
        auto* cell = new (vm.heap.allocate<__type>()) __type(std::forward<Args>(args)...); \
        cell->m_type = type(); \
        return cell; \
    }

class Value;

class Cell {
public:
    enum class Type : uint8_t {
        String,
        Array,
        Object,
        Function,
        InvalidCell,
    };

    template<typename T>
    T* cast()
    {
        ASSERT(m_type == T::type(), "Invalid cast");
        return reinterpret_cast<T*>(this);
    }

    virtual void visit(std::function<void(Value)>) const = 0;
    virtual void dump(std::ostream& out) const = 0;

    bool isMarked;

protected:
    Type m_type { Type::InvalidCell };
};
