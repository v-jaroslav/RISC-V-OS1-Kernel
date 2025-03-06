#include "k_tcb_sleep_queue.hpp"
#include "k_scheduler.hpp"

namespace Kernel {
    void TCBSleepQueue::put_to_sleep(TCB* tcb, time_t sleep_for) {
        // If we haven't passed tcb, then just ignore this operation.
        if (!tcb) {
            return;
        }

        // If we did pass tcb, then set its sleep_for ticks, change its state, and set its next and prev pointers to null.
        tcb->sleep_for = sleep_for;
        tcb->status = TCBStatus::SUSPENDED;
        TCBSleepQueue::unlink(tcb);

        // If our queue is empty, then this tcb will be the very first element.
        if (this->is_empty()) {
            this->set_first(tcb);
            return;
        }

        // We need to find the proper place where to place the tcb, and we up to that place need to sum the relative number of ticks that threads sleep for, to get absolute ticks of some threads.
        // As we will sum those relative ticks, at the thread we landed on and of which we added sleep_for ticks, basically in that sum total_time, from it we will get the absolute time left for that thread to sleep.
        size_t total_time = 0;
        TCB* current = this->head;

        while (current) {
            // Go through TCBs in the queue, and sum the number of relative ticks that they should sleep for.
            total_time += current->sleep_for;

            if (tcb->sleep_for >= total_time) {
                // In case the new TCB should sleep longer than so far summed time, then continue going through the queue.
                current = current->next;
            }
            else {
                // In case the new TCB should sleep less than so far sumemd time, then we are going to store it as the predecessor of the "current tcb".
                // From the sum of time, we subtract the sleep_for time of the "current tcb" to reverse the operation when we added that time.
                // That way we will have absolute time for how long the current old predecessor of "current tcb" should sleep for.
                total_time -= current->sleep_for;

                // Chain the new TCB to the queue, with relative sleeping time (by subtracting from its time the total_time).
                tcb->next = current;
                tcb->prev = current->prev;
                if (tcb->prev) {
                    tcb->prev->next = tcb;
                    tcb->sleep_for -= total_time;
                }
                else {
                    this->head = tcb;
                }

                // Update the predecessor of the current TCB and its sleeping time.
                current->sleep_for -= tcb->sleep_for;
                current->prev = tcb;
                break;
            }
        }

        if (!tcb->next) {
            // In case the current TCB does not have successor, that means it hasn't been added to the queue.
            // Therefore we couldn't find old tcb in the queue that would sleep longer than this new one we are inserting.
            // So it should be last tcb, so we will add it as last one.
            tcb->sleep_for -= total_time;
            this->add_last(tcb);
        }
    }

    void TCBSleepQueue::timer_tick() {
        if (!this->is_empty()) {
            // In case the queue for sleeping isn't empty, take the first TCB, and decrement its ticks for sleeping by one.
            this->peek_first()->sleep_for--;

            while (!this->is_empty() && this->peek_first()->sleep_for <= 0) {
                // As long as the queue is not empty, and as long as the time of the first TCB has expired, take that TCB and add it to the scheduler.
                Scheduler::get_instance().put_tcb(this->take_first());
            }
        }
    }
}