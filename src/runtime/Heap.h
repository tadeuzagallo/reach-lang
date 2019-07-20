#pragma once

#include "Allocator.h"
#include <queue>
#include <unordered_map>

class Cell;
class Value;
class VM;

using Visitor = std::function<void(Value)>;

class Heap {
public:
    Heap(VM*);

    template<typename CellType>
    void* allocate()
    {
		Allocator& allocator = Allocator::forSize(m_vm, sizeof(CellType));
		void* cell = allocator.cell();
		if (cell)
			return cell;

		collect();
		cell = allocator.cell();
		ASSERT(cell, "OOM: failed to allocate");
		return cell;
    }

    void addRoot(Cell*);
    void removeRoot(Cell*);

private:
    void collect();
    void markFromRoots();
    void markNativeStack(const std::function<void(Value)>&);
    void markInterpreterStack(const std::function<void(Value)>&);
    void sweep();
    void mark();
    bool isMarked(Cell*);
    void setMarked(Cell*);
    void clearMarked(Cell*);

    VM* m_vm;
    std::queue<Cell*> m_worklist;
    std::vector<Cell*> m_roots;
};
