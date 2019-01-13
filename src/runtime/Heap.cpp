#include "Heap.h"

#include "BytecodeBlock.h"
#include "Cell.h"
#include "VM.h"

Heap::Heap(VM* vm)
    : m_vm(vm)
{
}

void Heap::collect()
{
    if (std::getenv("NO_GC"))
        return;

    markFromRoots();
    sweep();
}

void Heap::markFromRoots()
{
    auto markRoot = [&](Value root) {
        if (root.isCrash() || !root.isCell())
            return;

        Cell* cell = root.asCell();
        if (isMarked(cell))
            return;

        setMarked(cell);
        m_worklist.push(cell);
        mark();
    };

    m_vm->globalBlock->visit(markRoot);
    m_vm->globalEnvironment.visit(markRoot);

    for (auto value : m_vm->stack) {
        markRoot(value);
    }
}

void Heap::sweep()
{
    Allocator::each([&](Allocator& allocator) {
        allocator.each([&](Cell* cell) {
            if (isMarked(cell)) {
                clearMarked(cell);
                return;
            }

            // TODO
            //cell->~Cell();
            allocator.free(cell);
        });
    });
}

void Heap::mark()
{
    while (!m_worklist.empty()) {
        Cell* cell = m_worklist.front();
        m_worklist.pop();

        cell->visit([&](Value value) {
            if (value.isCrash() || !value.isCell())
                return;

            Cell* cell = value.asCell();
            if (isMarked(cell))
                return;

            setMarked(cell);
            m_worklist.push(cell);
        });
    }
}

bool Heap::isMarked(Cell* cell)
{
    return cell->isMarked;
}

void Heap::setMarked(Cell* cell)
{
    cell->isMarked = true;
}

void Heap::clearMarked(Cell* cell)
{
    cell->isMarked = false;
}
