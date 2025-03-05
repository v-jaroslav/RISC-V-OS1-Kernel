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
    // Uzmi tekucu nit, postavi joj status da je suspendovana, i dodaj je u red suspendovanih niti.
    TCB* suspended_tcb = current_tcb;
    suspended_tcb->status = TCBStatus::SUSPENDED;
    this->suspended.add_last(suspended_tcb);

    // Uzmi neku novu nit, i uradi promenu konteksta.
    current_tcb = Scheduler::get_instance().next_tcb();
    yield(suspended_tcb, current_tcb);
}

int Sem::wait() {
    // Uzmi tekucu vrednost semafora, smanji je za 1.
    this->value = this->value - 1;

    if(this->value < 0) {
        // Ako je sada vrednost manja od 0, blokiraj tekucu nit.
        this->block();

        if(current_tcb->interrupted) {
            // Ukoliko se vracamo iz block-a, zato sto se semafor gasi, vrati gresku.
            return WAIT_FAIL;
        }
    }

    return WAIT_SUCCESS;
}

int Sem::unblock() {
    // Uzmi neku nit iz reda suspendovanih.
    TCB* tcb = this->suspended.take_first();

    if(tcb) {
        // Ako smo odblokirali nit, postavi joj interrupted zastavicu na true ako se semafor gasi, stavi je u red spremnih niti.
        tcb->interrupted = this->closing;
        Scheduler::get_instance().put_tcb(tcb);
        return UNBLOCK_SUCCESS;
    }

    return UNBLOCK_FAIL;
}

int Sem::signal() {
    // Uzmi tekucu vrednost semafora i uvecaj je za 1.
    this->value = this->value + 1;

    if(value <= 0) {
        // Ukoliko je vrednost bila negativna, deblokiraj jednu nit.
        this->unblock();
    }

    return SIGNAL_SUCCESS;
}

int Sem::close() {
    // Postavi zastavicu da smo ugasili semafor.
    this->closing = true;

    // Deblokiraj sve niti.
    while(this->unblock() == UNBLOCK_SUCCESS);
    return CLOSE_SUCCESS;
}
