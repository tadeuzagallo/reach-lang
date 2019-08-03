#pragma once

#include "Allocator.h"
#include <queue>
#include <unordered_map>

class Cell;
class Heap;
class VM;
class Value;

class Visitor {
    friend class Heap;

public:
    void visit(Value) const;

private:
    Visitor(Heap& heap)
        : m_heap(heap)
    {
    }

    void visitConservatively(Value) const;
    Cell* getCell(Value) const;
    void visitCell(Cell*) const;

    Heap& m_heap;
};

class Heap {
    friend class Visitor;

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
    void markRoot(Value);
    void markNativeStack();
    void markInterpreterStack();
    void sweep();
    void mark();
    bool isMarked(Cell*);
    void setMarked(Cell*);
    void clearMarked(Cell*);

    VM* m_vm;
    Visitor m_visitor;
    std::queue<Cell*> m_worklist;
    std::vector<Cell*> m_roots;
};
