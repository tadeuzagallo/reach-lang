#pragma once

#include <limits>
#include <queue>
#include <unordered_map>

class Cell;

class Allocator {
public:
    static Allocator& forSize(size_t);
    static void each(std::function<void(Allocator&)>);

    ~Allocator();

    Cell* cell();
    void free(Cell*);
    void each(std::function<void(Cell*)>);

private:
    constexpr static uint8_t s_freeMarker = std::numeric_limits<uint8_t>::max();

    static std::unordered_map<size_t, Allocator*> s_allocators;

    Allocator(size_t);

    size_t m_cellSize;
    size_t m_blockSize;
    uint8_t* m_start;
    uint8_t* m_end;
    uint8_t* m_current;
    std::queue<Cell*> m_freeList;
};
