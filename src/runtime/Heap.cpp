#include "Heap.h"

#include "BytecodeBlock.h"
#include "Cell.h"
#include "Environment.h"
#include "Interpreter.h"
#include "VM.h"

Heap::Heap(VM* vm)
    : m_vm(vm)
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
    auto markRoot = [&](Value root) {
        if (root.isCrash() || (!root.isCell() && !root.isAbstractValue()))
            return;

        Cell* cell = root.getCell();
        if (isMarked(cell))
            return;

        setMarked(cell);
        m_worklist.push(cell);
        mark();
    };

    markRoot(m_vm->stringType);
    markRoot(m_vm->typeType);
    markRoot(m_vm->topType);
    markRoot(m_vm->bottomType);
    markRoot(m_vm->unitType);
    markRoot(m_vm->boolType);
    markRoot(m_vm->numberType);
    markRoot(m_vm->globalEnvironment);
    for (Cell* root : m_roots)
        markRoot(root);
    if (m_vm->currentInterpreter)
        m_vm->currentInterpreter->visit(markRoot);
    if (m_vm->globalBlock)
        m_vm->globalBlock->visit(markRoot);

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
        if (root->isCrash() || (!root->isCell() && !root->isAbstractValue()))
            continue;

        Cell* cell = root->getCell();
        bool isValid = false;
        Allocator::each([&](Allocator& allocator) {
            if (isValid)
                return;
            if (allocator.contains(cell))
                isValid = true;
        });

        if (isValid && cell->m_kind < Cell::Kind::InvalidCell)
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
            if (value.isCrash() || (!value.isCell() && !value.isAbstractValue()))
                return;

            Cell* cell = value.getCell();
            if (isMarked(cell))
                return;

            setMarked(cell);
            m_worklist.push(cell);
        });
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
