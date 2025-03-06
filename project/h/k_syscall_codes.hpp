namespace Kernel {
    // Success/failure codes.
    constexpr int SUCCESS_SYSCALL =  0;
    constexpr int FAILED_SYSCALL  = -1;

    // System call codes (important for ABI)
    constexpr int MEM_ALLOC_CODE = 0x01;
    constexpr int MEM_FREE_CODE  = 0x02;

    constexpr int CREATE_THREAD_CODE   = 0x11;
    constexpr int THREAD_EXIT_CODE     = 0x12;
    constexpr int THREAD_DISPATCH_CODE = 0x13;
    constexpr int THREAD_JOIN_CODE     = 0x14;

    constexpr int SEM_OPEN_CODE   = 0x21;
    constexpr int SEM_CLOSE_CODE  = 0x22;
    constexpr int SEM_WAIT_CODE   = 0x23;
    constexpr int SEM_SIGNAL_CODE = 0x24;

    constexpr int TIME_SLEEP_CODE = 0x31;

    constexpr int GET_C_CODE = 0x41;
    constexpr int PUT_C_CODE = 0x42;

    // Additional system call, to switch to user mode.
    constexpr int USER_MODE_CODE = 0xFF;
}
