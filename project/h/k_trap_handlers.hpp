#pragma once

#include "../lib/hw.h"
#include "k_sem.hpp"

namespace Kernel 
{
    constexpr int INSTRUCTION_SIZE = 4;
    constexpr int IO_BUFFER_SIZE = 2048;

    extern Sem putc_sem;
    extern Sem getc_sem;

    void flush_putc_buffer();
    void fill_getc_buffer();

    extern "C" void k_handle_ecall(uint64 syscall_code, uint64 p0, uint64 p1, uint64 p2, uint64 p3, uint64 p4, uint64 p5, uint64 p6);
    extern "C" void k_handle_hardware(uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6, uint64 a7);

    void prepare_user_mode();
}
