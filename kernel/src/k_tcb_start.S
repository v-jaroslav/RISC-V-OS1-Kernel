.global k_tcb_start
.type k_tcb_start, @function

// Uvezi simbole za pocetnu tacku niti, i za tekuci kontekst.
.extern k_tcb_run_wrapper
.extern k_current_context

k_tcb_start:
    // Predji na korisnicki stek.
    csrw sscratch, x1
    ld x1, k_current_context
    sd sp, 0x10(x1)
    ld sp, 0x08(x1)
    csrr x1, sscratch

    // Ucitaj adresu simbola k_tcb_starting_point u ra.
    la ra, k_tcb_run_wrapper

    // U sepc upisi adresu ra registra, i izadji iz prekidne rutine.
    csrw sepc, ra
    sret
