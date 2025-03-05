#pragma once

#include "../lib/hw.h"

namespace Kernel {
    // Every register in RISC-V architecture has 64 bit size.
    typedef volatile uint64 Register;

    struct Context {
                          // RegisterName    Offset (in this Context struct)
        Register ra;      // x1              (0x00)

        Register usr_sp;  // x2              (0x08)
        Register sys_sp;  // x2              (0x10)

        Register gp;      // x3              (0x18)
        Register tp;      // x4              (0x20)

        Register s11;     // x27             (0x28)
        Register s10;     // x26             (0x30)
        Register s9;      // x25             (0x38)
        Register s8;      // x24             (0x40)
        Register s7;      // x23             (0x48)
        Register s6;      // x22             (0x50)
        Register s5;      // x21             (0x58)
        Register s4;      // x20             (0x60)
        Register s3;      // x19             (0x68)
        Register s2;      // x18             (0x70)
        Register s1;      // x9              (0x78)
        Register s0;      // x8 (fp)         (0x80)

        Register t6;      // x31             (0x88)
        Register t5;      // x30             (0x90)
        Register t4;      // x29             (0x98)
        Register t3;      // x28             (0xa0)
        Register t2;      // x7              (0xa8)
        Register t1;      // x6              (0xb0)
        Register t0;      // x5              (0xb8)

        Register a7;      // x17             (0xc0)
        Register a6;      // x16             (0xc8)
        Register a5;      // x15             (0xd0)
        Register a4;      // x14             (0xd8)
        Register a3;      // x13             (0xe0)
        Register a2;      // x12             (0xe8)
        Register a1;      // x11             (0xf0)
        Register a0;      // x10             (0xf8)

        Register sepc;    //                 (0x100)
        Register sstatus; //                 (0x108)
    };
}
