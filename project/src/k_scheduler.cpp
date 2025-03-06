#include "k_scheduler.hpp"
#include "k_trap_handlers.hpp"
#include "syscall_c.hpp"

namespace Kernel {
    static void flush_putc_loop(void* args) {
        while (true) {
            // This is used by internal thread, that will flush characters to the console, it always tries to execute thread_dispatch, so that other threads can run.
            Kernel::flush_putc_buffer();
            thread_dispatch();
        }
    }

    void Scheduler::initialize() {
        if (!this->flush_tcb) {
            // Create internal thread for flushing putc console buffer.
            thread_create((thread_t*)&this->flush_tcb, flush_putc_loop, nullptr);
        }
    }

    Scheduler& Scheduler::get_instance() {
        static Scheduler scheduler;
        return scheduler;
    }

    TCB* Scheduler::next_tcb() {
        return this->queue.take_first();
    }

    void Scheduler::put_tcb(TCB* tcb) {
        if (tcb) {
            tcb->status = TCBStatus::READY;
            this->queue.add_last(tcb);
        }
    }
}