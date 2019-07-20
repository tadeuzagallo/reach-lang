#pragma once

#include "Heap.h"

template<typename T>
class GC {
public:
    template<typename... Args>
    GC(VM& vm, Args&&... args)
        : m_ptr(T::create(vm, std::forward<Args>(args)...))
    {
        ASSERT(m_ptr, "OOPS");
        vm.heap.addRoot(m_ptr);
    }

    ~GC()
    {
        ASSERT(!m_ptr, "OOPS");
    }

    T* get() const { return m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }

    void destroy(VM& vm)
    {
        ASSERT(m_ptr, "OOPS");
        vm.heap.removeRoot(m_ptr);
        m_ptr = nullptr;
    }

private:

    T* m_ptr;
};
