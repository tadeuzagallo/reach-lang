#include "Array.h"

template<typename T, typename = std::enable_if_t<std::is_base_of_v<Cell, T>>>
class CellArray : public Array { 
public:
    CELL_CREATE(CellArray);

    T* getIndex(uint32_t index)
    {
        return Array::getIndex(index).template asCell<T>();
    }

    void setIndex(uint32_t index, Value value)
    {
        ASSERT(value.isCell<T>(), "Adding value of wrong type to CellArray");
        Array::setIndex(index, value);
    }

    void setIndex(uint32_t index, T* value)
    {
        Array::setIndex(index, value);
    }

    class iterator {
        friend CellArray<T>;

    public:
        bool operator!=(iterator& other) { return m_iterator != other.m_iterator; }
        bool operator==(iterator& other) { return m_iterator == other.m_iterator; }
        iterator& operator++() { ++m_iterator; return *this; }

        T* operator*() { return m_iterator->asCell<T>(); }
        T* operator->() { return m_iterator->asCell<T>(); }

    private:
        explicit iterator(std::vector<Value>::iterator iterator)
            : m_iterator(iterator)
        {
        }

        std::vector<Value>::iterator m_iterator;
    };

    iterator begin() { return iterator { Array::begin() }; }
    iterator end() { return iterator { Array::end() }; }
    //const_iterator begin() const { return Array::begin(); }
    //const_iterator end() const { return Array::end(); }

private:
    CellArray(uint32_t initialSize)
        : Array(nullptr, initialSize)
    {
    }

    CellArray(const std::vector<T>& vector)
        : Array(nullptr, vector)
    {
    }

    CellArray(uint32_t itemCount, const Value* items)
        : Array(nullptr, itemCount)
    {
        while (itemCount--)
            setIndex(itemCount, items[itemCount]);
    }

};
