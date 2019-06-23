#pragma once

#include "Allocator.h"
#include <iostream>

#define CELL_TYPE(__kind) \
    static constexpr Cell::Kind kind() { return Cell::Kind::__kind; }

#define CELL_CREATE(__type) \
    template<typename... Args> \
    static __type* create(VM& vm, Args&&... args) \
    { \
        auto* cell = new (vm.heap.allocate<__type>()) __type(std::forward<Args>(args)...); \
        cell->m_kind = kind(); \
        return cell; \
    }

#define CELL_CREATE_VM(__type) \
    template<typename... Args> \
    static __type* create(VM& vm, Args&&... args) \
    { \
        auto* cell = new (vm.heap.allocate<__type>()) __type(vm, std::forward<Args>(args)...); \
        cell->m_kind = kind(); \
        return cell; \
    }

#define CELL(__type) \
    CELL_TYPE(__type) \
    CELL_CREATE(__type) \

class Value;
class VM;

class Cell {
    friend class Heap;

public:
    enum class Kind : uint8_t {
        Object = 0x1,
        String = 0x2,
        Array = 0x4,
        Function = 0x8,
        Environment = 0x10,
        Type = 0x20 | Object,
        InvalidCell = 0x22,
    };

    template<typename T>
    bool is()
    {
        return static_cast<uint8_t>(m_kind) & static_cast<uint8_t>(T::kind());
    }

    template<typename T>
    T* cast()
    {
        ASSERT(is<T>(), "Invalid cast");
        return reinterpret_cast<T*>(this);
    }

    VM& vm() const
    {
        uintptr_t cell = reinterpret_cast<uintptr_t>(this);
        cell &= ~Allocator::s_blockMask;
        return *reinterpret_cast<Allocator::Header*>(cell)->vm;
    }

    virtual ~Cell() = default;

    virtual void visit(std::function<void(Value)>) const = 0;
    virtual void dump(std::ostream& out) const = 0;

    bool isMarked;

protected:
    Kind m_kind { Kind::InvalidCell };
};
