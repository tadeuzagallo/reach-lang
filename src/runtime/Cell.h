#pragma once

#include "Allocator.h"
#include <iostream>

class CellCastAllowed;

template<typename T>
T cell_cast_allowed_class(CellCastAllowed(T::*)());

template<typename T>
struct is_cell_cast_allowed : std::bool_constant<
                      std::is_base_of_v<Cell, T> &&
                      std::is_same_v<decltype(cell_cast_allowed_class(&T::cellCastAllowed)), T>
                      > { };

template<typename T>
static constexpr bool is_cell_cast_allowed_v = is_cell_cast_allowed<T>::value;

#define ALLOW_CELL_CAST() \
    CellCastAllowed cellCastAllowed()

#define CELL_TYPE(__kind) \
    static constexpr Cell::Kind kind() { return Cell::Kind::__kind; } \
    ALLOW_CELL_CAST();

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

class VM;
class Value;
class Visitor;

class Cell {
    friend class Heap;
    friend class JIT;
    friend class Visitor;

public:
    enum class Kind : uint32_t {
        Typed       = 0x1,
        Object      = 0x2 | Typed,
        String      = 0x4 | Typed,
        Array       = 0x8 | Typed,
        Function    = 0x10 | Typed,
        Tuple       = 0x20 | Typed,
        Type        = 0x40 | Object,
        Hole        = 0x80 | Type,
        Environment = 0x100,
        BytecodeBlock = 0x200,
    };

    static constexpr uint32_t KindMask
        = static_cast<uint32_t>(Kind::Typed)
        | static_cast<uint32_t>(Kind::Object)
        | static_cast<uint32_t>(Kind::String)
        | static_cast<uint32_t>(Kind::Array)
        | static_cast<uint32_t>(Kind::Function)
        | static_cast<uint32_t>(Kind::Tuple)
        | static_cast<uint32_t>(Kind::Type)
        | static_cast<uint32_t>(Kind::Hole)
        | static_cast<uint32_t>(Kind::Environment)
        | static_cast<uint32_t>(Kind::BytecodeBlock)
        ;

    Kind kind() const { return m_kind; }

    template<typename T, typename = std::enable_if_t<is_cell_cast_allowed_v<T>>>
    bool is()
    {
        return (static_cast<uint8_t>(m_kind) & static_cast<uint8_t>(T::kind())) == static_cast<uint8_t>(T::kind());
    }

    template<typename T, typename = std::enable_if_t<is_cell_cast_allowed_v<T>>>
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

    virtual void dump(std::ostream& out) const = 0;

protected:
    virtual void visit(const Visitor&) const = 0;

    bool m_isMarked;
    Kind m_kind { 0 };
};

std::ostream& operator<<(std::ostream&, Cell::Kind);
