#include "../h/k_scheduler.hpp"
#include "../h/k_trap_handlers.hpp"
#include "../h/syscall_c.hpp"

using namespace Kernel;

static void flush_putc_loop(void* args) {
    while(true) {
        // Sluzi za nit koja treba da salje karaktere konzoli.
        Kernel::flush_putc_buffer();
        thread_dispatch();
    }
}

void Scheduler::initialize() {
    if(!this->flush_tcb) {
        // Kreiraj internu nit jezgra za flushovanje putc buffera.
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
