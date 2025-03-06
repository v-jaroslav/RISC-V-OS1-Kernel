// Macro definitions that are used by multiple interrupt trap handlers.

.macro switch_to_kernel_stack
    // Save x1 to tmp sscratch register, to x1 load address of k_current_context struct, to context store usr_sp, and from it load sys_sp, and restore x1 from sscratch.
    csrw sscratch, x1
    ld x1, k_current_context
    sd sp, 0x08(x1)
    ld sp, 0x10(x1)
    csrr x1, sscratch
.endm
    
.macro switch_to_user_stack
    // Save x1 to tmp sscratch register, load to x1 address of k_current_context struct, to context store sys_sp, from context load usr_sp, and restore x1 from sscratch.
    csrw sscratch, x1
    ld x1, k_current_context
    sd sp, 0x10(x1)
    ld sp, 0x08(x1)
    csrr x1, sscratch
.endm

.macro switch_to_user_stack_ecall
    // Do the same thing as switch_to_user_stack, but also while you are at it, load from context the a0 register, the result of ecall operation.
    csrw sscratch, x1
    ld x1, k_current_context
    sd sp, 0x10(x1)
    ld sp, 0x08(x1)
    ld a0, 0xf8(x1)
    csrr x1, sscratch
.endm


.macro save_cpu_registries_on_stack
    // Allocate the space on the stack, and save the CPU registers to it.
    addi sp, sp, -240

    // All of the registers are 64 bit, so they have 8 bytes, therefore that is why we increment the offset by that amount.
    .set offset, 0
    .irp reg, ra, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, a0, a1, a2, a3, a4, a5, a6, a7, t0, t1, t2, t3, t4, t5, t6, gp, tp
        sd \reg, offset(sp)
        .set offset, offset + 8
    .endr
.endm
    
.macro restore_cpu_registries_from_stack
    // Restore CPU registers from the stack, and deallocate the space on the stack.
    .set offset, 0
    .irp reg, ra, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, a0, a1, a2, a3, a4, a5, a6, a7, t0, t1, t2, t3, t4, t5, t6, gp, tp
        ld \reg, offset(sp)
        .set offset, offset + 8
    .endr

    addi sp, sp, 240
.endm
    


// Import external symbols for functions that are supposed to handle the interrupts, and symbol that represents the current context.
.extern k_handle_ecall
.extern k_handle_timer
.extern k_handle_console
.extern k_current_context



// Export the system call trap handler function.
.global k_ecall_trap
.type k_ecall_trap, @function

k_ecall_trap:
    switch_to_kernel_stack
    save_cpu_registries_on_stack

    // Handle the system call.
    call k_handle_ecall

    restore_cpu_registries_from_stack
    switch_to_user_stack_ecall
    sret
    

// Export the timer trap handler function.
.global k_timer_trap
.type k_timer_trap, @function

k_timer_trap:
    switch_to_kernel_stack
    save_cpu_registries_on_stack

    // Handle the timer interrupt.
    call k_handle_timer

    restore_cpu_registries_from_stack
    switch_to_user_stack
    sret


// Export the console trap handler function.
.global k_console_trap
.type k_console_trap, @function

k_console_trap:
    switch_to_kernel_stack
    save_cpu_registries_on_stack

    // Handle the console interrupt.
    call k_handle_console

    restore_cpu_registries_from_stack
    switch_to_user_stack
    sret
