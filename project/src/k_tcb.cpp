#include "k_tcb.hpp"
#include "k_memory.hpp"
#include "k_scheduler.hpp"
#include "syscall_c.hpp"
#include "k_utils.hpp"

namespace Kernel {
    // Main thread of the kernel, statically allocated (that way HEAP_START_ADDR is moved implicitly).
    TCB main_tcb { { 0 }, nullptr, nullptr, false, nullptr, 0, DEFAULT_TIME_SLICE, TCBStatus::RUNNING, nullptr, nullptr, nullptr, nullptr };

    // Remember that the current context and current thread should point to the main thread and its context at the beginning.
    TCB* current_tcb = &Kernel::main_tcb;
    Context* k_current_context = &Kernel::main_tcb.context;

    // Initally, 0 ticks have passed since the last context switch.
    time_t volatile timer_ticks = 0;


    TCB* create_tcb(void (*body)(void* args), void* args, uint64* stack_space) {
        if (body && stack_space) {
            // We will create a new thread, only if the function that should be executed is passed, and if stack for that function is passed as well.
            TCB* new_tcb = (TCB*)MemoryAllocator::get_instance().alloc(Utils::to_blocks(sizeof(TCB)));
            if (!new_tcb) {
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
            new_tcb->sys_stack = (uint64*)MemoryAllocator::get_instance().alloc(Utils::to_blocks(DEFAULT_STACK_SIZE));
            if (!new_tcb->sys_stack) {
                free_tcb(new_tcb);
                return nullptr;
            }
            
            // Again, for the same reason why usr_sp is pointing to the &stack[last_index + 1], so will sys_sp.
            new_tcb->context.sys_sp = (uint64)&new_tcb->sys_stack[DEFAULT_STACK_SIZE / sizeof(uint64)];

            // Set the thraed_pointer registry to 0 (because of plic functions from hw.h, they require that).
            new_tcb->context.tp = 0;

            // Set the initial sstatus, it is inherited from the parent thread, enable interrupts regardless of that.
            // We are setting SPIE (SuperVisor Previous Interrupt Enable) bit, 5th one, that way when we return from suprevisor trap, interrupts will be enabled.
            __asm__ volatile ("csrr %0, sstatus" : "=r" (new_tcb->context.sstatus));
            new_tcb->context.sstatus = new_tcb->context.sstatus | (1 << 5);

            // Write to the context where we are going back to after the first context restauration of this new thread.
            // Which is code written in assembly, which redirects us to k_tcb_run_wrapper. We can't return directly to k_tcb_run_wrapper.
            // As we will be doing context switch in kernel, so we are still in supervisor trap, so we have to execute sret at one point to leave that.
            new_tcb->context.ra = (uint64)k_tcb_start;

            // Create new semamphore for join operation.
            new_tcb->join_sem = Kernel::Sem::create_sem(0);
            if (!new_tcb->join_sem) {
                free_tcb(new_tcb);
                return nullptr;
            }

            return new_tcb;
        }

        return nullptr;
    }

    void free_tcb(TCB* tcb) {
        if (tcb && tcb != &main_tcb) {
            // Deallocate the user stack, kernel stack, and semaphore, after that also TCB.
            // We know the usr_stack is basically address of &stack[last_index + 1], so we have to push it backwards to get original address.
            MemoryAllocator::get_instance().free((void*)((uint64)tcb->usr_stack - DEFAULT_STACK_SIZE));
            tcb->usr_stack = nullptr;

            // Unlinke usr_stack, sys_stack points to &stack[0], as we allocated it in kernel.
            MemoryAllocator::get_instance().free((void*)tcb->sys_stack);
            tcb->sys_stack = nullptr;

            Sem::free_sem(tcb->join_sem);
            tcb->join_sem = nullptr;

            MemoryAllocator::get_instance().free(tcb);
        }
    }

    void yield(TCB* old_tcb, TCB* new_tcb) {
        if (old_tcb && k_save_context(&old_tcb->context) != 0) {
            // In case old_tcb is passed, save its context. 
            // In case the return value is not 0, then we are returning from context restauration.
            // Because when we saved the context, we also saved "ra" return address registry. And when we are restoring the context.
            // Then we will return where we were supposed to after that, which is here. In that case, set the time ticks to be 0.
            // As it has been 0 time ticks ever since the last context switch, as it just happened.
            timer_ticks = 0;
            return;
        }

        if (new_tcb) {
            // If new_tcb is passed, set its status that its running, restore its context. We came here if we truly saved context of old_tcb.
            // However, we might return from k_save_context for new_tcb with result that is not zero, in case we have previously saved the context for new_tcb some time in the past.
            new_tcb->status = TCBStatus::RUNNING;
            k_current_context = &current_tcb->context;
            k_restore_context(&new_tcb->context);
        }
    }

    void dispatch() {
        TCB* previous_tcb = current_tcb;

        if (previous_tcb && previous_tcb->status == TCBStatus::RUNNING) {
            // If we have previous thread, and if it is not done, add it back to the scheduler.
            Scheduler::get_instance().put_tcb(previous_tcb);
        }
        else if (previous_tcb && previous_tcb->status == TCBStatus::TERMINATING) {
            // If it did finish, deallocate it, but before that, close its semaphore for join operations.
            if (previous_tcb->join_sem) {
                previous_tcb->join_sem->close();
            }
            free_tcb(previous_tcb);
            previous_tcb = nullptr;
        }

        // Pick different thread from scheduler, switch context.
        current_tcb = Scheduler::get_instance().next_tcb();
        yield(previous_tcb, current_tcb);
    }
    
    extern "C" void k_tcb_run_wrapper() {
        // Start the thread with the given arguments, once it is done, call the system call to shut down the thread.
        // This function will not be name mangled in C++ way, so that we can use it from the assembly easily.
        current_tcb->body(current_tcb->args);
        thread_exit();
    }
}