#include "Allocator.h"
#include "Assert.h"

std::unordered_map<size_t, Allocator*> Allocator::s_allocators;

Allocator::Allocator(size_t cellSize)
    : m_cellSize(cellSize)
{
    m_blockSize = m_cellSize * 256;
    m_start = reinterpret_cast<uint8_t*>(malloc(m_blockSize));
    ASSERT(m_start, "Failed to create allocation block");
    m_current = m_start;
    m_end = m_start + m_blockSize;
}

Allocator::~Allocator()
{
    ::free(m_start);
}

Allocator& Allocator::forSize(size_t size)
{
    auto it = s_allocators.find(size);
    if (it != s_allocators.end())
        return *it->second;
    Allocator* allocator = new Allocator(size);
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

    if (m_current < m_end) {
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
