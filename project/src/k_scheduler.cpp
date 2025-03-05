#include "../h/k_scheduler.hpp"
#include "../h/k_trap_handlers.hpp"
#include "../h/syscall_c.hpp"

using namespace Kernel;

static void flush_putc_loop(void* args) {
    while(true) {
        // This is used by system thread, that will send characters to the console, it always tries to execute thread_dispatch, so that other threads can run.
        Kernel::flush_putc_buffer();
        thread_dispatch();
    }
}

void Scheduler::initialize() {
    if(!this->flush_tcb) {
        // Create internal system thread for flushing putc console buffer.
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
    if(tcb) {
        tcb->status = TCBStatus::READY;
        this->queue.add_last(tcb);
    }
}
