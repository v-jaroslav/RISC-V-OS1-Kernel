// Align the interrupt on 4 bytes.
.align 4

// Export the symbol, for the interrupt vector table.
.global k_intr_table
.type k_intr_table, @function

// Import the symbols for interrupt handlers.
.extern k_ecall_trap
.extern k_hardware_trap

k_intr_table:
    // Entry for ecall / exceptions.
    jal zero, k_ecall_trap

    // Entry for timer interrupt.
    .org k_intr_table + 1 * 4
    jal zero, k_hardware_trap

    // Entry for console interrupt.
    .org k_intr_table + 9 * 4
    jal zero, k_hardware_trap
