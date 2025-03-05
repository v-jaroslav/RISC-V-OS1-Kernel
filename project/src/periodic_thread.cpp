#include "../h/syscall_cpp.hpp"

PeriodicThread::PeriodicThread(time_t period) : Thread([](void* t) {
        PeriodicThread* p_thr = (PeriodicThread*)t;
        while(p_thr && p_thr->period > 0) {
            p_thr->periodicActivation();
            time_sleep(p_thr->period);
        }
    }, this) {
    this->period = period;
}

void PeriodicThread::terminate() {
    this->period = (time_t)0;
    this->join();
}
