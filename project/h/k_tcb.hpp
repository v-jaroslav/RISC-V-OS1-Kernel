#pragma once

#include "hw.h"
#include "k_context.hpp"
#include "k_sem.hpp"

namespace Kernel {
    // States in which we can find some thread in.
    enum class TCBStatus { INITIALIZING, SUSPENDED, TERMINATING, READY, RUNNING };

    struct TCB {
        // Context of the thread, all the registers of the CPU are here (which includes stack pointer that points to the stack of the thread, etc).
        Context context;

        // All threads have two stacks, user stack (used for running the user program), and system kernel stack (used for kernel operations).
        // In case user stack is full, we can still execute kernel operations as we have kernel stack.
        uint64* usr_stack;
        uint64* sys_stack;

        // Used to know whether to return WAIT_FAILURE or not from semaphore.
        bool interrupted;
        Sem* join_sem;

        // For how long should the thread sleep, what is its timeslice, and status.
        time_t sleep_for;
        time_t time_slice;
        TCBStatus status;

        // What function to run the thread on, and what arguments to pass to that function.
        void (*body)(void* args);
        void* args;

        // We have these two pointers here so we can use TCB as element of a linked list.
        TCB* next;
        TCB* prev;
    };

    // External declarations for the main thread of main() function, and currently running thread.
    // And also external declaration for current context, yes we can read it from current_tcb, but its easier to have it declared like this as well, so that we can deal with it easier in assembly.
    extern TCB main_tcb;
    extern TCB* current_tcb;
    extern "C" Context* k_current_context;

    // How many ticks have passed since the last context switch.
    extern time_t volatile timer_ticks;

    TCB* create_tcb(void (*body)(void* args), void* args, uint64* stack_space);
    void free_tcb(TCB* tcb);

    void yield(TCB* old_tcb, TCB* new_tcb);
    void dispatch();

    // Function from which thread starts.
    extern "C" void k_tcb_run_wrapper();

    // Functions that are defined in RISC-V assembly, so we have external declaration to them.
    extern "C" int  k_save_context(Context* context);
    extern "C" int  k_restore_context(Context* context);
    extern "C" void k_tcb_start();
}
