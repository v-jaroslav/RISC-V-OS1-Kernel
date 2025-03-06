// Export k_tcb_start as global symbol.
.global k_tcb_start
.type k_tcb_start, @function

// Import symbols for the thread starting point, and symbol that represents the current context.
.extern k_tcb_run_wrapper
.extern k_current_context

k_tcb_start:
    // Switch to user stack. We store x1 to temporary sscratch registry.
    // And after that to x1 we load address of current context, to which we store sys_sp, and load usr_sp, and after that we load back x1 from sscratch.
    csrw sscratch, x1
    ld x1, k_current_context
    sd sp, 0x10(x1)
    ld sp, 0x08(x1)
    csrr x1, sscratch

    // Load address of symbol k_tcb_run_wrapper to ra register. So that we return there.
    la ra, k_tcb_run_wrapper

    // To sepc register write address from ra register, and leave the interrupt handle routine.
    csrw sepc, ra
    sret
