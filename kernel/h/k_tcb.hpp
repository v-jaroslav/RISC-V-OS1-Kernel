#pragma once

#include "../lib/hw.h"
#include "k_context.hpp"
#include "k_sem.hpp"

namespace Kernel {
    // States in which we can find some thread in.
    enum class TCBStatus { INITIALIZING, SUSPENDED, TERMINATING, READY, RUNNING };

    struct TCB {
        Context context;

        // All threads have two stacks, user stack (used for running user program), and system stack (used for kernel operations).
        // This makes our kernel multi-threaded kernel.
        uint64* usr_stack;
        uint64* sys_stack;

        bool interrupted;
        Sem* join_sem;

        time_t sleep_for;
        time_t time_slice;
        TCBStatus status;

        void (*body)(void* args);
        void* args;

        TCB* next;
        TCB* prev;
    };

    extern TCB main_tcb;
    extern TCB* current_tcb;
    extern "C" Context* k_current_context;

    extern time_t volatile timer_ticks;

    TCB* create_tcb(void (*body)(void* args), void* args, uint64* stack_space);
    void free_tcb(TCB* tcb);

    void yield(TCB* old_tcb, TCB* new_tcb);
    void dispatch();

    // Function from which thread starts.
    extern "C" void k_tcb_run_wrapper();

    // Functions that are defined in RISC-V assembly.
    extern "C" int k_save_context(Context* context);
    extern "C" int k_restore_context(Context* context);
    extern "C" void k_tcb_start();
}
