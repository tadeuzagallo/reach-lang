#pragma once

#include "Allocator.h"
#include <queue>
#include <unordered_map>

class Cell;
class VM;

class Heap {
public:
    Heap(VM*);

    template<typename CellType>
    void* allocate()
    {
		Allocator& allocator = Allocator::forSize(sizeof(CellType));
		void* cell = allocator.cell();
		if (cell)
			return cell;

		collect();
		cell = allocator.cell();
		ASSERT(cell, "OOM: failed to allocate");
		return cell;
    }

private:
    void collect();
    void markFromRoots();
    void sweep();
    void mark();
    bool isMarked(Cell*);
    void setMarked(Cell*);
    void clearMarked(Cell*);

    VM* m_vm;
    std::queue<Cell*> m_worklist;
};
