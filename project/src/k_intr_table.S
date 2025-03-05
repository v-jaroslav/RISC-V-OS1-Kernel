// Poravnaj prekidnu rutinu na 4 bajta.
.align 4

// Izvezi simbol, za interrupt vector tabelu.
.global k_intr_table
.type k_intr_table, @function

// Uvezi simbole prekidnih rutina.
.extern k_ecall_trap
.extern k_hardware_trap

k_intr_table:
    // Ulaz za ecall / exception-e.
    jal zero, k_ecall_trap

    // Ulaz za tajmer.
    .org k_intr_table + 1 * 4
    jal zero, k_hardware_trap

    // Ulaz za konzolu.
    .org k_intr_table + 9 * 4
    jal zero, k_hardware_trap
