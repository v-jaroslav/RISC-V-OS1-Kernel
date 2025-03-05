.global k_restore_context
.type k_restore_context, @function

k_restore_context:
    // Restauriraj sledece registre iz strukture Context.
    ld ra, 0x00(a0)

    // Procitaj sys_sp.
    ld sp, 0x10(a0)

    ld gp, 0x18(a0)
    ld tp, 0x20(a0)

    ld s11, 0x28(a0)
    ld s10, 0x30(a0)
    ld s9, 0x38(a0)
    ld s8, 0x40(a0)
    ld s7, 0x48(a0)
    ld s6, 0x50(a0)
    ld s5, 0x58(a0)
    ld s4, 0x60(a0)
    ld s3, 0x68(a0)
    ld s2, 0x70(a0)
    ld s1, 0x78(a0)
    ld s0, 0x80(a0)

    ld t6, 0x88(a0)
    ld t5, 0x90(a0)
    ld t4, 0x98(a0)
    ld t3, 0xa0(a0)
    ld t2, 0xa8(a0)
    ld t1, 0xb0(a0)
    ld t0, 0xb8(a0)

    ld a7, 0xc0(a0)
    ld a6, 0xc8(a0)
    ld a5, 0xd0(a0)
    ld a4, 0xd8(a0)
    ld a3, 0xe0(a0)
    ld a2, 0xe8(a0)

    ld a1, 0x100(a0)
    csrw sepc, a1

    ld a1, 0x108(a0)
    csrw sstatus, a1

    ld a1, 0xf0(a0)

    // U a0 (registar povratne vrednosti), pre nego sto se vratis, upisi vrednost 1, kako bi oznacio da se vracas iz restauracije konteksta.
    // Ova funkcija ce da nas vrati na mesto gde bi se vratio poziv iz k_save_context-a, po uzoru na longjmp.
    lui a0, 0x0000000000000001
    ret
