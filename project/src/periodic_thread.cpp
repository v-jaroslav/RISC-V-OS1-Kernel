#include "syscall_cpp.hpp"

PeriodicThread::PeriodicThread(time_t period) 
    : Thread(
        [](void* t) {
            // Repeatedly run the periodicActivation in case the period at which it should be run is greater than zero, and then sleep for the given period.
            PeriodicThread* p_thr = (PeriodicThread*)t;
            while (p_thr && p_thr->period > 0) {
                p_thr->periodicActivation();
                time_sleep(p_thr->period);
            }
        }, 
        this
    ) {
    this->period = period;
}

void PeriodicThread::terminate() {
    // Terminate the thread, by setting the period, the rate at which the periodicActivation should be called, to zero.
    // That way in case it is about to run another periodicActivation(), we at least don't want the thread to sleep again.
    // And wait for it to finish.
    this->period = (time_t)0;
    this->join();
}
