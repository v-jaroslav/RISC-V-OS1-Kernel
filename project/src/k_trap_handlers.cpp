#include "../h/k_trap_handlers.hpp"
#include "../h/k_syscall_codes.hpp"
#include "../h/k_tcb_sleep_queue.hpp"
#include "../h/k_scheduler.hpp"
#include "../h/k_memory.hpp"
#include "../h/syscall_c.hpp"
#include "../h/queue.hpp"
#include "../h/k_utils.hpp"

using namespace Kernel;

static TCBSleepQueue sleep_queue;

static Queue<char, IO_BUFFER_SIZE> putc_buffer;
Sem Kernel::putc_sem;

static Queue<char, IO_BUFFER_SIZE> getc_buffer;
Sem Kernel::getc_sem;

void Kernel::flush_putc_buffer() {
    // Read the current value of sstatus register.
    uint64 volatile sstatus_val;
    __asm__ volatile ("csrr %[sts], sstatus" : [sts] "=r" (sstatus_val));

    // To bit of index 1 (SIE) of sstatus registry, write 0, so that we mask the interrupt because of critical section.
    __asm__ volatile ("csrc sstatus, 0x02");

    while(putc_buffer.get_length() > 0 && *((uint8*)CONSOLE_STATUS) & CONSOLE_TX_STATUS_BIT) {
        *((char*)CONSOLE_TX_DATA) = putc_buffer.get();
        putc_sem.signal();
    }

    // Write back the old value of sstatus.
    __asm__ volatile ("csrw sstatus, %[sts]" : : [sts] "r" (sstatus_val));
}

void Kernel::fill_getc_buffer() {
    if(*((uint8*)CONSOLE_STATUS) & CONSOLE_RX_STATUS_BIT) {
        char temp = *((char*)CONSOLE_RX_DATA);
        if(getc_buffer.get_length() < getc_buffer.get_capacity()) {
            // In case buffer has space, store character inside of it, otherwise ignore. Perform signalization, as maybe someone waits on character.
            getc_buffer.put(temp);
            getc_sem.signal();
            dispatch();
        }
    }
}

extern "C" void Kernel::k_handle_ecall(uint64 syscall_code, uint64 p0, uint64 p1, uint64 p2, uint64 p3, uint64 p4, uint64 p5, uint64 p6) {
    uint64 volatile scause_val, sepc_val, temp_val;
    __asm__ volatile("csrr %0, scause" : "=r" (scause_val));
    __asm__ volatile("csrr %0, sepc" : "=r" (sepc_val));

    if(scause_val == 8 || scause_val == 9) {
        // SEPC contains address of ecall (points to ecall instruction), so increment it by the size of the instruction to point to the instruction after ecall.
        sepc_val += INSTRUCTION_SIZE;
        __asm__ volatile ("csrw sepc, %0" : : "r" (sepc_val));

        // In SIP registry, to the 2-nd bit SSIP write 0, with that we say we handled the software interrupt.
        __asm__ volatile("csrc sip, 0x02");

        // At the start, we assume the system call has failed. If it did, we will later write the code of success.
        k_current_context->a0 = FAILED_SYSCALL;

        switch(syscall_code) {
            case MEM_ALLOC_CODE:
                k_current_context->a0 = (uint64)MemoryAllocator::get_instance().alloc(p0);
                break;

            case MEM_FREE_CODE:
                k_current_context->a0 = MemoryAllocator::get_instance().free((void*)p0);
                break;

            case CREATE_THREAD_CODE:
                if((_thread**)p0) {
                    // Create new thread, only if you have place to store its handle.
                    *((_thread**)p0) = (_thread*)create_tcb((void (*)(void*))p1, (void*)p2, (uint64*)p3);
                    if(*((_thread**)p0)) {
                        Scheduler::get_instance().put_tcb(*((TCB**)p0));
                        k_current_context->a0 = SUCCESS_SYSCALL;
                    }
                }
                break;

            case THREAD_EXIT_CODE:
                if(current_tcb != &main_tcb) {
                    current_tcb->status = TCBStatus::TERMINATING;
                    k_current_context->a0 = SUCCESS_SYSCALL;
                    dispatch();
                }
                break;

            case THREAD_DISPATCH_CODE:
                dispatch();
                break;

            case THREAD_JOIN_CODE:
                if((TCB*)p0 && ((TCB*)p0)->join_sem) {
                    // In case we have pointer to the TCB, and if it has its semaphore, then perform wait on that semaphore.
                    ((TCB*)p0)->join_sem->wait();
                }
                break;

            case SEM_OPEN_CODE:
                if((_sem**)p0) {
                    // Create semaphore only if you can save the handle of it.
                    *((_sem**)p0) = (_sem*)Sem::create_sem((int)p1);
                    if(*((_sem**)p0)) {
                        k_current_context->a0 = SUCCESS_SYSCALL;
                    }
                }
                break;

            case SEM_CLOSE_CODE:
                if((Sem*)p0) {
                    temp_val = ((Sem*)p0)->close();
                    if(Sem::free_sem((Sem*)p0) == MemoryAllocator::MEM_SUCCESS) {
                        k_current_context->a0 = temp_val;
                    }
                }
                break;

            case SEM_WAIT_CODE:
                if((Sem*)p0) {
                    k_current_context->a0 = ((Sem*)p0)->wait();
                }
                break;

            case SEM_SIGNAL_CODE:
                if((Sem*)p0) {
                    k_current_context->a0 = ((Sem*)p0)->signal();
                }
                break;

            case TIME_SLEEP_CODE:
                if(p0 > 0) {
                    // Let thread sleep, but only if number of ticks to sleep for are greater than 0.
                    sleep_queue.put_to_sleep(current_tcb, p0);
                    k_current_context->a0 = SUCCESS_SYSCALL;
                    dispatch();
                }
                break;

            case GET_C_CODE:
                getc_sem.wait();
                k_current_context->a0 = getc_buffer.get();
                break;

            case PUT_C_CODE:
                putc_sem.wait();
                putc_buffer.put((char)p0);
                break;

            case USER_MODE_CODE:
                prepare_user_mode();
                k_current_context->a0 = SUCCESS_SYSCALL;
                break;
        }
    }
    else if (scause_val == 2 || scause_val == 5 || scause_val == 7){
        // Empty everything that we have in putc buffer to the console.
        flush_putc_buffer();

        // Write specific error message through buffer for putc, based on the scause code. Indeces are: (2 - 2) / 2 = 0; (5 - 2) / 2 = 1; (7 - 2) / 2 = 2;
        const char* messages[3] = {"KERNEL PANIC! ILLEGAL INSTRUCTION AT: ", "KERNEL PANIC! ILLEGAL READ OPERATION AT: ", "KERNEL PANIC! ILLEGAL WRITE OPERATION AT: "};
        for(int i = 0; messages[(scause_val - 2) / 2][i] != '\0'; i++) {
            putc_buffer.put(messages[(scause_val - 2) / 2][i]);
        }

        uint64 tens = Utils::get_decimal_weight(sepc_val);
        while(tens) {
            // Perform integer division on "number" with "tens", take remainder when dividing by 10, and divide "tens" later with 10, so that we take digits from left to right.
            putc_buffer.put('0' + (sepc_val / tens) % 10);
            tens = tens / 10;
        }
        putc_buffer.put('\n');

        flush_putc_buffer();
        while(true);
    }
}

extern "C" void Kernel::k_handle_hardware(uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6, uint64 a7) {
    uint64 volatile scause_val;
    __asm__ volatile("csrr %0, scause" : "=r" (scause_val));

    if(scause_val == 0x8000000000000001UL) {
        // In SIP registry, write to the 2nd bit SSIP 0, with that we say that we handled the software interrupt.
        __asm__ volatile("csrc sip, 0x02");

        // Asynchronous context change.
        sleep_queue.timer_tick();
        if(++timer_ticks >= current_tcb->time_slice) {
            dispatch();
        }
    }
    else if (scause_val == 0x8000000000000009UL) {
        // Grab the interrupt entry number.
        int irq = plic_claim();

        // To 9-th bit SEIP write 0, in SIP registry, with that we say that we did handle the hardware interrupt.
        __asm__ volatile("csrc sip, %0" : : "r" (0x200));

        // Tell the PLIC (platform level interrupt controller), that the interrupt has been handled, so that it wont interrupt us again for it.
        plic_complete(irq);

        fill_getc_buffer();
    }
}

void Kernel::prepare_user_mode() {
    uint64 volatile sstatus_val;
    __asm__ volatile ("csrr %[sts], sstatus" : [sts] "=r" (sstatus_val));

    // Set the 8-th bit to 0, which represents previous mode (now its user mode), with sret we go back to the previous mode.
    __asm__ volatile("csrw sstatus, %[sts]" : : [sts] "r" (sstatus_val & ~(1 << 8)));
}
