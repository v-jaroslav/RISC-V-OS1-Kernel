// Uvezi simbol, za funkciju koja treba da obradi prekid, i tekuci kontekst.
.extern k_handle_ecall
.extern k_current_context

.global k_ecall_trap
.type k_ecall_trap, @function

k_ecall_trap:
    // Predji na sistemski stek.
    csrw sscratch, x1
    ld x1, k_current_context
    sd sp, 0x08(x1)
    ld sp, 0x10(x1)
    csrr x1, sscratch

    // Alociraj na steku prostor. Sacuvaj korisnicke registre na stek.
    addi sp, sp, -240

    sd ra, 0x00(sp)

    sd s0, 0x08(sp)
    sd s1, 0x10(sp)
    sd s2, 0x18(sp)
    sd s3, 0x20(sp)
    sd s4, 0x28(sp)
    sd s5, 0x30(sp)
    sd s6, 0x38(sp)
    sd s7, 0x40(sp)
    sd s8, 0x48(sp)
    sd s9, 0x50(sp)
    sd s10, 0x58(sp)
    sd s11, 0x60(sp)

    sd a0, 0x68(sp)
    sd a1, 0x70(sp)
    sd a2, 0x78(sp)
    sd a3, 0x80(sp)
    sd a4, 0x88(sp)
    sd a5, 0x90(sp)
    sd a6, 0x98(sp)
    sd a7, 0xa0(sp)

    sd t0, 0xa8(sp)
    sd t1, 0xb0(sp)
    sd t2, 0xb8(sp)
    sd t3, 0xc0(sp)
    sd t4, 0xc8(sp)
    sd t5, 0xd0(sp)
    sd t6, 0xd8(sp)

    sd gp, 0xe0(sp)
    sd tp, 0xe8(sp)

    // Opsluzi sistemski poziv.
    call k_handle_ecall

    // Restauriraj korisnicke registre sa steka. I dealociraj prostor na steku.
    ld ra, 0x00(sp)

    ld s0, 0x08(sp)
    ld s1, 0x10(sp)
    ld s2, 0x18(sp)
    ld s3, 0x20(sp)
    ld s4, 0x28(sp)
    ld s5, 0x30(sp)
    ld s6, 0x38(sp)
    ld s7, 0x40(sp)
    ld s8, 0x48(sp)
    ld s9, 0x50(sp)
    ld s10, 0x58(sp)
    ld s11, 0x60(sp)

    ld a0, 0x68(sp)
    ld a1, 0x70(sp)
    ld a2, 0x78(sp)
    ld a3, 0x80(sp)
    ld a4, 0x88(sp)
    ld a5, 0x90(sp)
    ld a6, 0x98(sp)
    ld a7, 0xa0(sp)

    ld t0, 0xa8(sp)
    ld t1, 0xb0(sp)
    ld t2, 0xb8(sp)
    ld t3, 0xc0(sp)
    ld t4, 0xc8(sp)
    ld t5, 0xd0(sp)
    ld t6, 0xd8(sp)

    ld gp, 0xe0(sp)
    ld tp, 0xe8(sp)

    addi sp, sp, 240

    // Predji na korisnicki stek, procitaj rezultat ecall poziva.
    csrw sscratch, x1
    ld x1, k_current_context
    sd sp, 0x10(x1)
    ld sp, 0x08(x1)
    ld a0, 0xf8(x1)
    csrr x1, sscratch

    sret
