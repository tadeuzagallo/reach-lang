#include "Heap.h"

#include "BytecodeBlock.h"
#include "Cell.h"
#include "VM.h"

void Visitor::visit(Value value) const
{
    if (Cell* cell = getCell(value))
        visitCell(cell);
}

void Visitor::visitConservatively(Value value) const
{
    if (Cell* cell = getCell(value)) {
        bool isValid = false;
        Allocator::each([&](Allocator& allocator) {
            if (allocator.contains(cell)) {
                isValid = true;
                return IterationResult::Stop;
            }
            return IterationResult::Continue;
        });

        if (!isValid)
            return;
        uint32_t kind = static_cast<uint32_t>(cell->m_kind);
        if (kind && (kind & Cell::KindMask) == kind)
            visitCell(cell);
    }
}

Cell* Visitor::getCell(Value value) const
{
    if (value.isCrash() || (!value.isCell() && !value.isAbstractValue()))
        return nullptr;

    return value.getCell();
}

void Visitor::visitCell(Cell* cell) const
{
    if (m_heap.isMarked(cell))
        return;

    m_heap.setMarked(cell);
    m_heap.m_worklist.push(cell);
}

Heap::Heap(VM* vm)
    : m_vm(vm)
    , m_visitor(*this)
{
}

void Heap::addRoot(Cell* cell)
{
    m_roots.emplace_back(cell);
}

void Heap::removeRoot(Cell* cell)
{
    auto it = std::find(m_roots.begin(), m_roots.end(), cell);
    ASSERT(it != m_roots.end(), "OOPS");
    m_roots.erase(it);
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
    for (Cell* root : m_roots)
        markRoot(root);

    m_vm->visit(m_visitor);
    markNativeStack();
    mark();
    markInterpreterStack();
}

void Heap::markRoot(Value root)
{
    m_visitor.visit(root);
    mark();
}

void Heap::markNativeStack()
{
    volatile void** rsp;
    asm("movq %%rsp, %0" : "=r"(rsp));
    pthread_t self = pthread_self();
    Value* stackBottom = reinterpret_cast<Value*>(pthread_get_stackaddr_np(self));
    for (Value* root = reinterpret_cast<Value*>(rsp); root != stackBottom; ++root)
        m_visitor.visitConservatively(*root);
}

void Heap::markInterpreterStack()
{
    for (auto value : m_vm->stack)
        markRoot(value);
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
        return IterationResult::Continue;
    });
}

void Heap::mark()
{
    while (!m_worklist.empty()) {
        Cell* cell = m_worklist.front();
        m_worklist.pop();

        cell->visit(m_visitor);
    }
}

bool Heap::isMarked(Cell* cell)
{
    return cell->m_isMarked;
}

void Heap::setMarked(Cell* cell)
{
    cell->m_isMarked = true;
}

void Heap::clearMarked(Cell* cell)
{
    cell->m_isMarked = false;
}
