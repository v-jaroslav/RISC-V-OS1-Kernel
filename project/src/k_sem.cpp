#include "k_sem.hpp"
#include "k_memory.hpp"
#include "k_scheduler.hpp"
#include "k_utils.hpp"

namespace Kernel {
    Sem* Sem::create_sem(int value) {
        Sem* new_sem = (Sem*)MemoryAllocator::get_instance().alloc(Utils::to_blocks(sizeof(Sem)));
        new_sem->initialize(value);
        return new_sem;
    }

    int Sem::free_sem(Sem* sem) {
        return MemoryAllocator::get_instance().free(sem);
    }

    void Sem::initialize(int value) {
        this->value = value;
        this->suspended_tcbs.initialize();
    }

    void Sem::block() {
        // Take the current thread, let its status be suspended, and add it to the queue of suspended threads.
        TCB* suspended_tcb = current_tcb;
        suspended_tcb->status = TCBStatus::SUSPENDED;
        this->suspended_tcbs.add_last(suspended_tcb);

        // Take a new thread, and perform context switch.
        current_tcb = Scheduler::get_instance().next_tcb();
        yield(suspended_tcb, current_tcb);
    }

    int Sem::wait() {
        // Take the current value of semaphore, decrement it by 1.
        this->value = this->value - 1;

        if (this->value < 0) {
            // If this value is now lesser than 0 (negative), block the current thread.
            this->block();

            if (current_tcb->interrupted) {
                // In case the context was switched to this function, and in case the tcb that is currently being run was interrupted from its waiting.
                // Then the wait has failed, and so return that as status code.
                return WAIT_FAIL;
            }
        }

        return WAIT_SUCCESS;
    }

    int Sem::unblock(bool wait_error) {
        // Take first thread from the suspended threads list.
        TCB* tcb = this->suspended_tcbs.take_first();

        if (tcb) {
            // If we truly found a thread to resume, then let its interrupted flag be true in case semaphore is closing, otherwise false.
            // Regardless, add this TCB to the scheduling queue.
            tcb->interrupted = wait_error;
            Scheduler::get_instance().put_tcb(tcb);
            return UNBLOCK_SUCCESS;
        }

        return UNBLOCK_FAIL;
    }

    int Sem::signal() {
        // Take the current value of semaphore, and increment it.
        this->value = this->value + 1;

        if(value <= 0) {
            // In case the value was negative (it is even if it is 0 now, as then it was -1 previously since we incremented it), resume one thread.
            // And resume one thread in a way that it returns WAIT_SUCCESS from its sem_wait().
            this->unblock(false);
        }

        return SIGNAL_SUCCESS;
    }

    int Sem::close() {
        // Resume all the blocked threads, such that they all return WAIT_FAIL from their sem_wait, once all of them are resumed, return.
        while (this->unblock(true) == UNBLOCK_SUCCESS);
        return CLOSE_SUCCESS;
    }
}