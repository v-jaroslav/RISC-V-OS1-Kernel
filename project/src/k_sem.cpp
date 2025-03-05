#include "../h/k_sem.hpp"
#include "../h/k_memory.hpp"
#include "../h/k_scheduler.hpp"
#include "../h/utils.hpp"

using namespace Kernel;

void Sem::initialize(int value) {
    this->value = value;
    this->closing = false;
    this->suspended.initialize();
}

Sem* Sem::create_sem(int value) {
    Sem* new_sem = (Sem*)MemoryAllocator::get_instance().alloc(to_blocks(sizeof(Sem)));
    new_sem->initialize(value);
    return new_sem;
}

int Sem::free_sem(Sem* sem) {
    return MemoryAllocator::get_instance().free(sem);
}

void Sem::block() {
    // Take the current thread, let its status be suspended, and add it to the queue of suspended threads.
    TCB* suspended_tcb = current_tcb;
    suspended_tcb->status = TCBStatus::SUSPENDED;
    this->suspended.add_last(suspended_tcb);

    // Take a new thread, and perform context switch.
    current_tcb = Scheduler::get_instance().next_tcb();
    yield(suspended_tcb, current_tcb);
}

int Sem::wait() {
    // Take the current value of semaphore, decrement it by 1.
    this->value = this->value - 1;

    if(this->value < 0) {
        // If this value is now lesser than 0, block the current thread.
        this->block();

        if(current_tcb->interrupted) {
            // In case the context was switched to this function, and we are about to return from it, which is in case of when semaphore is being freed, return error.
            return WAIT_FAIL;
        }
    }

    return WAIT_SUCCESS;
}

int Sem::unblock() {
    // Take some thread from suspended threads queue.
    TCB* tcb = this->suspended.take_first();

    if (tcb) {
        // In case we resumed thread, let its interrupted flag be true in case its semaphore is closing, and add it to the queue of ready threads.
        tcb->interrupted = this->closing;
        Scheduler::get_instance().put_tcb(tcb);
        return UNBLOCK_SUCCESS;
    }

    return UNBLOCK_FAIL;
}

int Sem::signal() {
    // Take the current value of semaphore, and increment it.
    this->value = this->value + 1;

    if(value <= 0) {
        // In case the value was negative, resume one thread.
        this->unblock();
    }

    return SIGNAL_SUCCESS;
}

int Sem::close() {
    // Set the flag that we have closed the semaphore.
    this->closing = true;

    // Resume all threads.
    while(this->unblock() == UNBLOCK_SUCCESS);
    return CLOSE_SUCCESS;
}
