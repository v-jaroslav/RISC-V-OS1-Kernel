#pragma once

#include "list.hpp"

namespace Kernel {
    // Forward declaration, to protect ourselves from circular dependency.
    class TCB;

    class Sem {
    private:
        int value;
        List<TCB> suspended_tcbs;

        void block();
        int unblock(bool wait_error);

    public:
        static Sem* create_sem(int value);
        static int free_sem(Sem* sem);

        void initialize(int value);

        int wait();
        int signal();
        int close();

        // Success/failure codes.
        constexpr static int UNBLOCK_FAIL    = -1;
        constexpr static int UNBLOCK_SUCCESS =  0;
        constexpr static int WAIT_FAIL       = -1;
        constexpr static int WAIT_SUCCESS    =  0;
        constexpr static int SIGNAL_SUCCESS  =  0;
        constexpr static int CLOSE_SUCCESS   =  0;
    };
}
