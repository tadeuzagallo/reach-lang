#pragma once

#include <iostream>

#define CELL_TYPE(__type) \
    static constexpr Cell::Type type() { return Cell::Type::__type; }

#define CELL_CREATE(__type) \
    template<typename... Args> \
    static __type* create(VM& vm, Args&&... args) \
    { \
        auto* cell = new (vm.heap.allocate<__type>()) __type(std::forward<Args>(args)...); \
        cell->m_type = type(); \
        return cell; \
    }

#define CELL(__type) \
    CELL_TYPE(__type) \
    CELL_CREATE(__type) \

class Value;

class Cell {
    friend class Heap;

public:
    enum class Type : uint8_t {
        String,
        Array,
        Object,
        Function,
        Type,
        InvalidCell,
    };

    template<typename T>
    bool is()
    {
        return m_type == T::type();
    }

    template<typename T>
    T* cast()
    {
        ASSERT(is<T>(), "Invalid cast");
        return reinterpret_cast<T*>(this);
    }

    virtual void visit(std::function<void(Value)>) const = 0;
    virtual void dump(std::ostream& out) const = 0;

    bool isMarked;

protected:
    Type m_type { Type::InvalidCell };
};
