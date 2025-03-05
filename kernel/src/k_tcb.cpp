#include "../h/k_tcb.hpp"
#include "../h/k_memory.hpp"
#include "../h/k_scheduler.hpp"
#include "../h/syscall_c.hpp"
#include "../h/utils.hpp"

using namespace Kernel;

// Glavna nit kernela, staticki kreirana.
TCB Kernel::main_tcb { { 0 }, nullptr, nullptr, false, nullptr, 0, DEFAULT_TIME_SLICE, TCBStatus::RUNNING, nullptr, nullptr, nullptr, nullptr };

// Postavi da tekuci kontekst i tekuca nit pokazuju na glavnu main nit odnosno njen kontekst.
TCB* Kernel::current_tcb = &Kernel::main_tcb;
Context* Kernel::k_current_context = &Kernel::main_tcb.context;

time_t volatile Kernel::timer_ticks = 0;


TCB* Kernel::create_tcb(void (*body)(void* args), void* args, uint64* stack_space) {
    if(body && stack_space) {
        // Ukoliko je prosledjena funkcija koju treba izvrsiti i prostor za njen stek, samo tada kreiramo nit.
        TCB* new_tcb = (TCB*)MemoryAllocator::get_instance().alloc(to_blocks(sizeof(TCB)));
        if(!new_tcb) {
            return nullptr;
        }

        // Postavi osnovne vrednosti polja TCB-a.
        new_tcb->sleep_for = 0;
        new_tcb->time_slice = DEFAULT_TIME_SLICE;
        new_tcb->status = TCBStatus::INITIALIZING;
        new_tcb->interrupted = false;
        new_tcb->body = body;
        new_tcb->args = args;
        new_tcb->next = nullptr;
        new_tcb->prev = nullptr;

        // Postavi prosledjen stek, kao korisnicki stek.
        new_tcb->usr_stack = stack_space;
        new_tcb->context.usr_sp = (uint64)stack_space;

        // Alociraj kernel stek, i postavi ga.
        new_tcb->sys_stack = (uint64*)MemoryAllocator::get_instance().alloc(to_blocks(DEFAULT_STACK_SIZE));
        if(!new_tcb->sys_stack) {
            free_tcb(new_tcb);
            return nullptr;
        }
        new_tcb->context.sys_sp = (uint64)&new_tcb->sys_stack[DEFAULT_STACK_SIZE / sizeof(uint64)];

        // Postavi thread-pointer na 0, zbog plic funkcija.
        new_tcb->context.tp = 0;

        // Postavi pocetni sstatus, nasledjuje se od roditeljske niti, omoguci prekide svakako.
        __asm__ volatile ("csrr %0, sstatus" : "=r" (new_tcb->context.sstatus));
        new_tcb->context.sstatus = new_tcb->context.sstatus | (1 << 5);

        // U context upisi gde se vracamo nakon prve restauracije konteksta nove niti.
        new_tcb->context.ra = (uint64)k_tcb_start;

        // Napravi semafor za join.
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
        // Dealociraj korisnicki stek, kernel stek, semafor pa TCB.
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
    // Porkreni nit sa datim argumentima, kad zavrsi pozovi sistemski poziv da se nit zavrsi.
    current_tcb->body(current_tcb->args);
    thread_exit();
}

void Kernel::yield(TCB* old_tcb, TCB* new_tcb) {
    if(old_tcb && k_save_context(&old_tcb->context) != 0) {
        // Ako je prosledjen old_tcb sacuvaj kontekst, ako je povratna vrednost razlicita od nule onda se vracamo ovde preko restauracije konteksta.
        timer_ticks = 0;
        return;
    }

    if (new_tcb) {
        // Ako je prosledjen new_tcb postavi status da se izvrsava, restauriraj kontekst. Ovde smo dosli ako smo zaista sacuvali kontekst old_tcb-a.
        new_tcb->status = TCBStatus::RUNNING;
        k_current_context = &current_tcb->context;
        k_restore_context(&new_tcb->context);
    }
}

void Kernel::dispatch() {
    TCB* previous_tcb = current_tcb;
    if(previous_tcb && previous_tcb->status == TCBStatus::RUNNING) {
        // Ako imamo prethodnu nit, i ako se nije zavrsila, vrati je u scheduler.
        Scheduler::get_instance().put_tcb(previous_tcb);
    }
    else if (previous_tcb && previous_tcb->status == TCBStatus::TERMINATING) {
        // Ako se zavrsila, dealociraj je, ali pre toga, zatvori semafor date niti.
        if(previous_tcb->join_sem) {
            previous_tcb->join_sem->close();
        }
        free_tcb(previous_tcb);
        previous_tcb = nullptr;
    }

    // Izaberi drugu neku nit iz scheduler-a. Promeni kontekst.
    current_tcb = Scheduler::get_instance().next_tcb();
    yield(previous_tcb, current_tcb);
}
