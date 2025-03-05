#include "../h/k_tcb.hpp"
#include "../h/k_memory.hpp"
#include "../h/k_scheduler.hpp"
#include "../h/syscall_c.hpp"
#include "../h/utils.hpp"

using namespace Kernel;

// Main thread of the kernel, statically allocated.
TCB Kernel::main_tcb { { 0 }, nullptr, nullptr, false, nullptr, 0, DEFAULT_TIME_SLICE, TCBStatus::RUNNING, nullptr, nullptr, nullptr, nullptr };

// Remember that the current context and current thread should point to the main thread, aka its context.
TCB* Kernel::current_tcb = &Kernel::main_tcb;
Context* Kernel::k_current_context = &Kernel::main_tcb.context;

time_t volatile Kernel::timer_ticks = 0;


TCB* Kernel::create_tcb(void (*body)(void* args), void* args, uint64* stack_space) {
    if(body && stack_space) {
        // We create new thread, only if the function that should be executed is passed, and the space for its stack.
        TCB* new_tcb = (TCB*)MemoryAllocator::get_instance().alloc(to_blocks(sizeof(TCB)));
        if(!new_tcb) {
            return nullptr;
        }

        // Initialize the fields of the TCB.
        new_tcb->sleep_for = 0;
        new_tcb->time_slice = DEFAULT_TIME_SLICE;
        new_tcb->status = TCBStatus::INITIALIZING;
        new_tcb->interrupted = false;
        new_tcb->body = body;
        new_tcb->args = args;
        new_tcb->next = nullptr;
        new_tcb->prev = nullptr;

        // Set the passed stack, as the user stack.
        new_tcb->usr_stack = stack_space;
        new_tcb->context.usr_sp = (uint64)stack_space;

        // Allocate the kernel stack and set it.
        new_tcb->sys_stack = (uint64*)MemoryAllocator::get_instance().alloc(to_blocks(DEFAULT_STACK_SIZE));
        if(!new_tcb->sys_stack) {
            free_tcb(new_tcb);
            return nullptr;
        }
        new_tcb->context.sys_sp = (uint64)&new_tcb->sys_stack[DEFAULT_STACK_SIZE / sizeof(uint64)];

        // Set the thraed_pointer to 0, because of plic functions.
        new_tcb->context.tp = 0;

        // Set the initial sstatus, it is inherited from the parent thread, enable interrupts regardless of that.
        __asm__ volatile ("csrr %0, sstatus" : "=r" (new_tcb->context.sstatus));
        new_tcb->context.sstatus = new_tcb->context.sstatus | (1 << 5);

        // Write to the context where we are going back to after the first context restauration of this new thread.
        new_tcb->context.ra = (uint64)k_tcb_start;

        // Create new semamphore for join operation.
        new_tcb->join_sem = Kernel::Sem::create_sem(0);
        if(!new_tcb->join_sem) {
            free_tcb(new_tcb);
            return nullptr;
        }

        return new_tcb;
    }

    return nullptr;
}

void Kernel::free_tcb(TCB* tcb) {
    if(tcb && tcb != &main_tcb) {
        // Deallocate the user stack, kernel stack, and semaphore, after that also TCB.
        MemoryAllocator::get_instance().free((void*)((uint64)tcb->usr_stack - DEFAULT_STACK_SIZE));
        tcb->usr_stack = nullptr;

        MemoryAllocator::get_instance().free((void*)tcb->sys_stack);
        tcb->usr_stack = nullptr;

        Sem::free_sem(tcb->join_sem);
        tcb->join_sem = nullptr;

        MemoryAllocator::get_instance().free(tcb);
    }
}

extern "C" void Kernel::k_tcb_run_wrapper() {
    // Start the thread with the given arguments, once it is done, call the system call to shut down the thread.
    current_tcb->body(current_tcb->args);
    thread_exit();
}

void Kernel::yield(TCB* old_tcb, TCB* new_tcb) {
    if(old_tcb && k_save_context(&old_tcb->context) != 0) {
        // In case old_tcb is passed, save its context, in case the return value is not 0, then we are returning from context restauration.
        timer_ticks = 0;
        return;
    }

    if (new_tcb) {
        // If new_tcb is passed, set its status that its running, restore context. We came here if we truly saved context of old_tcb.
        new_tcb->status = TCBStatus::RUNNING;
        k_current_context = &current_tcb->context;
        k_restore_context(&new_tcb->context);
    }
}

void Kernel::dispatch() {
    TCB* previous_tcb = current_tcb;
    if(previous_tcb && previous_tcb->status == TCBStatus::RUNNING) {
        // If we have previous thread, and if it is not done, add it back to the scheduler.
        Scheduler::get_instance().put_tcb(previous_tcb);
    }
    else if (previous_tcb && previous_tcb->status == TCBStatus::TERMINATING) {
        // If it did finish, deallocate it, but before that, close its semaphore for join operations.
        if(previous_tcb->join_sem) {
            previous_tcb->join_sem->close();
        }
        free_tcb(previous_tcb);
        previous_tcb = nullptr;
    }

    // Pick different thread from scheduler, switch context.
    current_tcb = Scheduler::get_instance().next_tcb();
    yield(previous_tcb, current_tcb);
}
