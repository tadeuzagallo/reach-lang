#include "Heap.h"

#include "BytecodeBlock.h"
#include "Cell.h"
#include "Environment.h"
#include "TypeChecker.h"
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

    m_vm->globalEnvironment->visit(markRoot);
    if (m_vm->globalBlock)
        m_vm->globalBlock->visit(markRoot);
    if (m_vm->typeChecker)
        m_vm->typeChecker->visit(markRoot);

    markNativeStack(markRoot);
    markInterpreterStack(markRoot);
}

void Heap::markNativeStack(const std::function<void(Value)>& visitor)
{
    volatile void** rsp;
    asm("movq %%rsp, %0" : "=r"(rsp));
    pthread_t self = pthread_self();
    Value* stackBottom = reinterpret_cast<Value*>(pthread_get_stackaddr_np(self));
    for (Value* root = reinterpret_cast<Value*>(rsp); root != stackBottom; ++root)  {
        if (root->isCrash() || !root->isCell())
            continue;

        Cell* cell = reinterpret_cast<Cell*>(root->m_bits);
        bool isValid = false;
        Allocator::each([&](Allocator& allocator) {
            if (isValid)
                return;
            if (allocator.contains(cell))
                isValid = true;
        });

        if (isValid && cell->m_type < Cell::Type::InvalidCell)
            visitor(*root);
    }
}

void Heap::markInterpreterStack(const std::function<void(Value)>& visitor)
{
    for (auto value : m_vm->stack)
        visitor(value);
}

void Heap::sweep()
{
    Allocator::each([&](Allocator& allocator) {
        allocator.each([&](Cell* cell) {
            if (isMarked(cell)) {
                clearMarked(cell);
                return;
            }

            cell->~Cell();
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
