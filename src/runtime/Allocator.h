#pragma once

#include <limits>
#include <queue>
#include <unordered_map>

class Cell;
class VM;

enum class IterationResult {
    Stop,
    Continue,
};

class Allocator {
    friend class Cell;

public:
    static Allocator& forSize(VM*, size_t);
    static void each(const std::function<IterationResult(Allocator&)>&);

    ~Allocator();

    Cell* cell();
    void free(Cell*);
    bool contains(const Cell*);
    void each(const std::function<void(Cell*)>&);

private:
    struct Header {
        VM* vm;
    };

    constexpr static uint8_t s_freeMarker = std::numeric_limits<uint8_t>::max();
    static constexpr size_t s_blockSize = 0x10000;
    static constexpr uintptr_t s_blockMask = s_blockSize - 1;

    static std::unordered_map<size_t, Allocator*> s_allocators;

    Allocator(VM*, size_t);

    size_t m_cellSize;
    Header* m_header;
    uint8_t* m_start;
    uint8_t* m_end;
    uint8_t* m_current;
    std::queue<Cell*> m_freeList;
};
