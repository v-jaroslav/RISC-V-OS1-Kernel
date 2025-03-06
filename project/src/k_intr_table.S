// Align the interrupt vector table to be on address that is divisible by 4.
.align 4

// Export the symbol for the interrupt vector table.
.global k_intr_table
.type k_intr_table, @function

// Import the symbols for interrupt handlers.
.extern k_ecall_trap
.extern k_timer_trap
.extern k_console_trap

k_intr_table:
    // Entry for ecall / exceptions. Jump and link, we jump to the specific address, and to x0 we write return address.
    // Which means we don't write anything to x0 since it is hard wired to zero.
    jal zero, k_ecall_trap

    // Entry for timer interrupt, we changed our origin (address) at which we will write the instruction.
    .org k_intr_table + 1 * 4
    jal zero, k_timer_trap

    // Entry for console interrupt.
    .org k_intr_table + 9 * 4
    jal zero, k_console_trap
