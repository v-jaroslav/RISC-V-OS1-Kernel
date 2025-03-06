#pragma once

#include "hw.h"
#include "k_sem.hpp"

namespace Kernel {
    // The size of instruction in RISC-V architecture, and size of the IO buffer for console.
    constexpr int INSTRUCTION_SIZE = 4;
    constexpr int IO_BUFFER_SIZE = 2048;
    
    constexpr int SCAUSE_ECALL_USER       = 8;
    constexpr int SCAUSE_ECALL_SUPERVISOR = 9;
    
    constexpr int SCAUSE_ILLEGAL_INSTRUCTION    = 2;
    constexpr int SCAUSE_LOAD_ACCESS_FAULT      = 5;
    constexpr int SCAUSE_STORE_AMO_ACCESS_FAULT = 7;

    // Semaphores on which threads may be suspended if the buffer is full/empty for putc/getc.
    extern Sem putc_sem;
    extern Sem getc_sem;

    void flush_putc_buffer();
    void fill_getc_buffer();

    extern "C" void k_handle_ecall(uint64 syscall_code, uint64 p0, uint64 p1, uint64 p2, uint64 p3, uint64 p4, uint64 p5, uint64 p6);
    extern "C" void k_handle_timer(uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6, uint64 a7);
    extern "C" void k_handle_console(uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6, uint64 a7);

    // This handles our user_mode set_user_mode() system call.
    void prepare_user_mode();
}
