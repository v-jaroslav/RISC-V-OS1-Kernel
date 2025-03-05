#pragma once

#include "k_tcb.hpp"
#include "list.hpp"

namespace Kernel {
    class Scheduler {
    private:
        List<TCB> queue;
        TCB* flush_tcb;

        Scheduler() = default;
        ~Scheduler() = default;

    public:
        static Scheduler& get_instance();

        Scheduler(const Scheduler&) = delete;
        Scheduler& operator=(const Scheduler&) = delete;

        void initialize();

        TCB* next_tcb();
        void put_tcb(TCB* tcb);
    };
}
