#pragma once

#include "hw.h"
#include "list.hpp"
#include "k_tcb.hpp"

namespace Kernel {
    class TCBSleepQueue : private List<TCB> {
    public:
        void put_to_sleep(TCB* tcb, time_t sleep_for);
        void timer_tick();
    };
}
