.global k_save_context
.type k_save_context, @function

k_save_context:
    // Sacuvaj sledece registre u tekuci kontekst.
    sd ra, 0x00(a0)

    // Sacuvaj sys_sp.
    sd sp, 0x10(a0)

    sd gp, 0x18(a0)
    sd tp, 0x20(a0)

    sd s11, 0x28(a0)
    sd s10, 0x30(a0)
    sd s9, 0x38(a0)
    sd s8, 0x40(a0)
    sd s7, 0x48(a0)
    sd s6, 0x50(a0)
    sd s5, 0x58(a0)
    sd s4, 0x60(a0)
    sd s3, 0x68(a0)
    sd s2, 0x70(a0)
    sd s1, 0x78(a0)
    sd s0, 0x80(a0)

    sd t6, 0x88(a0)
    sd t5, 0x90(a0)
    sd t4, 0x98(a0)
    sd t3, 0xa0(a0)
    sd t2, 0xa8(a0)
    sd t1, 0xb0(a0)
    sd t0, 0xb8(a0)

    sd a7, 0xc0(a0)
    sd a6, 0xc8(a0)
    sd a5, 0xd0(a0)
    sd a4, 0xd8(a0)
    sd a3, 0xe0(a0)
    sd a2, 0xe8(a0)
    sd a1, 0xf0(a0)

    // Sacuvaj SEPC preko a1.
    csrr a1, sepc
    sd a1, 0x100(a0)

    // Sacuvaj SSTATUS preko a1.
    csrr a1, sstatus
    sd a1, 0x108(a0)

    // Ucitaj nazad sacuvan a1.
    ld a1, 0xf0(a0)

    // U a0 (registar povratne vrednosti), pre nego sto se vratis, upisi vrednost 0, kako bi oznacio da se zaista vracas iz ove funkcije. Uzor na setjmp funkciju.
    lui a0, 0x0000000000000000
    ret
