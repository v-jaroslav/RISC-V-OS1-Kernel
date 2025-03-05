#include "../h/k_tcb_sleep_queue.hpp"
#include "../h/k_scheduler.hpp"

using namespace Kernel;

void TCBSleepQueue::put_to_sleep(TCB* tcb, time_t sleep_for) {
    if(!tcb) {
        return;
    }

    tcb->sleep_for = sleep_for;
    tcb->status = TCBStatus::SUSPENDED;
    TCBSleepQueue::unlink(tcb);

    if(this->is_empty()) {
        this->set_first(tcb);
        return;
    }

    size_t total_time = 0;
    TCB* current = this->head;

    while(current) {
        // Prodji kroz TCB-ove u redu, i sabiraj broj relativnih tickova koje oni treba da spavaju.
        total_time += current->sleep_for;

        if(tcb->sleep_for >= total_time) {
            // Ako nov TCB treba da spava duze od dosada sabranog vremena, nastavi da prolazis kroz red.
            current = current->next;
        }
        else {
            // Ako ce TCB spavati krace od sume vremena, stavljamo ga kao prethodnika poslednje posecenog TCB-a.
            // Od sume vremena, oduzimamo vreme spavanja poslednjeg posecenog TCB-a, cilj je pamtiti razliku vremena spavanja u odnosu na prethodnika.
            total_time -= current->sleep_for;

            // Ulancaj nov TCB u red, sa relativnim vremenom spavanja.
            tcb->next = current;
            tcb->prev = current->prev;
            if(tcb->prev) {
                tcb->prev->next = tcb;
                tcb->sleep_for -= total_time;
            }
            else {
                this->head = tcb;
            }

            // Azuriranje sledbenika novog TCB-a, jer se unelo dodatno vreme u red spavanja.
            current->sleep_for -= tcb->sleep_for;
            current->prev = tcb;
            break;
        }
    }

    if(!tcb->next) {
        // Ako TCB nema sledbenika, znaci da nije dodat u red.
        tcb->sleep_for -= total_time;
        this->add_last(tcb);
    }
}

void TCBSleepQueue::timer_tick() {
    if(!this->is_empty()) {
        // Ako red za spavanje nije prazan, uzmi prvi TCB, i smanji broj tick-ova za spavanje.
        this->peek_first()->sleep_for--;

        while(!this->is_empty() && this->peek_first()->sleep_for <= 0) {
            // Dok god red nije prazan, i dok god trenutnom prvom TCB-u je isteklo vreme spavanja, uzmi taj TCB i stavi ga u scheduler.
            Scheduler::get_instance().put_tcb(this->take_first());
        }
    }
}
