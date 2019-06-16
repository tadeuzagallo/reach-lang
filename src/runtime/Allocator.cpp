#include "Allocator.h"

#include "Assert.h"
#include <stdlib.h>

std::unordered_map<size_t, Allocator*> Allocator::s_allocators;

Allocator::Allocator(VM* vm, size_t cellSize)
    : m_cellSize(cellSize)
{
    Header* header;
    int result = posix_memalign(reinterpret_cast<void**>(&header), s_blockSize, s_blockSize);
    ASSERT(!result, "Failed to create allocation block");
    *header = Header { vm };
    m_start = reinterpret_cast<uint8_t*>(header + 1);
    m_current = m_start;
    m_end = m_start + s_blockSize - sizeof(Header);
}

Allocator::~Allocator()
{
    ::free(m_start);
}

Allocator& Allocator::forSize(VM* vm, size_t size)
{
    ASSERT(size < s_blockSize, "Allocation is too big: %lu", size);
    auto it = s_allocators.find(size);
    if (it != s_allocators.end())
        return *it->second;
    Allocator* allocator = new Allocator(vm, size);
    s_allocators[size] = allocator;
    return *allocator;
}

void Allocator::each(std::function<void(Allocator&)> functor)
{
    for (auto& pair : s_allocators)
        functor(*pair.second);
}

void Allocator::each(std::function<void(Cell*)> functor)
{
    for (uint8_t* cell = m_start; cell != m_current; cell += m_cellSize) {
        if (*cell == s_freeMarker)
            continue;
        functor(reinterpret_cast<Cell*>(cell));
    }
}

Cell* Allocator::cell()
{
    if (!m_freeList.empty()) {
        Cell* cell = m_freeList.front();
        m_freeList.pop();
        return cell;
    }

    if (m_current + m_cellSize <= m_end) {
        Cell* result = reinterpret_cast<Cell*>(m_current);
        m_current += m_cellSize;
        return result;
    }

    return nullptr;
}

void Allocator::free(Cell* cell)
{
    uint8_t* address = reinterpret_cast<uint8_t*>(cell);
    ASSERT(address >= m_start && address < m_current, "Cell does not belong to this allocator");
    // TODO: Debug only
    memset(address, 0, m_cellSize);
    *address = s_freeMarker;
    m_freeList.push(cell);
}

bool Allocator::contains(const Cell* cell)
{
    // TODO: quicker check based on alignment
    const uint8_t* address = reinterpret_cast<const uint8_t*>(cell);
    if (address < m_start || address >= m_current)
        return false;
    if ((address - m_start) % m_cellSize)
        return false;
    return true;
}
