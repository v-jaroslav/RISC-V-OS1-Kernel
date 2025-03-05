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
        // Go through TCBs in the queue, and sum the number of relative ticks that they should sleep for.
        total_time += current->sleep_for;

        if(tcb->sleep_for >= total_time) {
            // In case the new TCB should sleep longer than so far summed time, then continue going through the queue.
            current = current->next;
        }
        else {
            // In case the TCB should sleep less than so far sumemd time, then we are going to store it as predecessor of the last visited TCB.
            // From the sum of time, we subtract the time the last visited TCB would take to sleep, the goal is to remember the time difference compared to the predecessor.
            total_time -= current->sleep_for;

            // Chain the new TCB to the queue, with relative sleeping time.
            tcb->next = current;
            tcb->prev = current->prev;
            if(tcb->prev) {
                tcb->prev->next = tcb;
                tcb->sleep_for -= total_time;
            }
            else {
                this->head = tcb;
            }

            // Update the successor of the new TCB, because we have added new time to the sleeping queue.
            current->sleep_for -= tcb->sleep_for;
            current->prev = tcb;
            break;
        }
    }

    if(!tcb->next) {
        // In case the current TCB does not have successor, that means it hasn't been added to the queue.
        tcb->sleep_for -= total_time;
        this->add_last(tcb);
    }
}

void TCBSleepQueue::timer_tick() {
    if(!this->is_empty()) {
        // In case the queue for sleeping isn't empty, take the first TCB, and decrement its ticks for sleeping by one.
        this->peek_first()->sleep_for--;

        while(!this->is_empty() && this->peek_first()->sleep_for <= 0) {
            // As long as the queue is not empty, and as long as the time of the first TCB has expired, take that TCB and add it to the scheduler.
            Scheduler::get_instance().put_tcb(this->take_first());
        }
    }
}
