#include "k_trap_handlers.hpp"
#include "k_syscall_codes.hpp"
#include "k_tcb_sleep_queue.hpp"
#include "k_scheduler.hpp"
#include "k_memory.hpp"
#include "syscall_c.hpp"
#include "queue.hpp"
#include "k_utils.hpp"

namespace Kernel {
    // Queue in which we will store threads that requested to sleep for some time.
    static TCBSleepQueue sleep_queue;

    // Buffer of characters that wait to be printed to the screen, and semaphore for it.
    static Queue<char, IO_BUFFER_SIZE> putc_buffer;
    Sem putc_sem;

    // Buffer of characters that wait to be read by threads, and semaphore for it.
    static Queue<char, IO_BUFFER_SIZE> getc_buffer;
    Sem getc_sem;

    void flush_putc_buffer() {
        // Read the current value of sstatus register.
        uint64 volatile sstatus_val;
        __asm__ volatile ("csrr %[sts], sstatus" : [sts] "=r" (sstatus_val));

        // To bit of index 1 (SIE Software Interrupt Enable) of SSTATUS registry, write 0, so that we mask the interupts because of the critical section.
        __asm__ volatile ("csrc sstatus, 0x02");

        while (putc_buffer.get_length() > 0 && *((uint8*)CONSOLE_STATUS) & CONSOLE_TX_STATUS_BIT) {
            // As long as we have characters in buffer to flush, and as long as we can transmit (TX) char to the console, then take one char from the buffer.
            // And write it to the CONSOLE_TX_DATA register that is memory mapped, therefore we can access it with pointer. And wake up one thread if there is one that is waiting to print.
            *((char*)CONSOLE_TX_DATA) = putc_buffer.get();
            putc_sem.signal();
        }

        // Write back the old value of sstatus. Unlock critical section.
        __asm__ volatile ("csrw sstatus, %[sts]" : : [sts] "r" (sstatus_val));
    }

    void fill_getc_buffer() {
        if (*((uint8*)CONSOLE_STATUS) & CONSOLE_RX_STATUS_BIT) {
            // In case console wants us to read (RX) character, then read one character from memory mapped CONSOLE_RX_DATA register.
            char temp = *((char*)CONSOLE_RX_DATA);
            
            if (getc_buffer.get_length() < getc_buffer.get_capacity()) {
                // In case buffer has space, store character inside of it, otherwise ignore. Perform signal operation on semaphore, as maybe some thread waits on character to read.
                // And in that case, try to switch thread, as this is urgent operation! We want to handle inputs as fast as possible.
                getc_buffer.put(temp);
                getc_sem.signal();
                dispatch();
            }
        }
    }

    extern "C" void k_handle_ecall(uint64 syscall_code, uint64 p0, uint64 p1, uint64 p2, uint64 p3, uint64 p4, uint64 p5, uint64 p6) {
        // Read current SCAUSE (SuperVisor Cause) and SEPC (SuperVisor Exception Program Counter).
        uint64 volatile scause_val, sepc_val, temp_val;
        __asm__ volatile("csrr %0, scause" : "=r" (scause_val));
        __asm__ volatile("csrr %0, sepc" : "=r" (sepc_val));

        if (scause_val == SCAUSE_ECALL_USER || scause_val == SCAUSE_ECALL_SUPERVISOR) {
            // SEPC contains address of ecall (points to the ecall instruction), so increment it by the size of the instruction to point to the instruction after ecall.
            // If we didn't do this, we would be basically stuck in an infinite loop. As after returning from ecall trap, we would again jump right into it.
            sepc_val += INSTRUCTION_SIZE;
            __asm__ volatile ("csrw sepc, %0" : : "r" (sepc_val));

            // In SIP (SuperVisor Interrupt Pending) registry, to the 2nd bit SSIP (SuperVisor Software Interrput Pending) write 0, with that we say we handled the software interrupt.
            __asm__ volatile("csrc sip, 0x02");

            // At the start, we assume that the system call has failed. If it didn't, we will later write the code of success.
            k_current_context->a0 = FAILED_SYSCALL;

            switch (syscall_code) {
                case MEM_ALLOC_CODE:
                    k_current_context->a0 = (uint64)MemoryAllocator::get_instance().alloc(p0);
                    break;

                case MEM_FREE_CODE:
                    k_current_context->a0 = MemoryAllocator::get_instance().free((void*)p0);
                    break;

                case CREATE_THREAD_CODE:
                    if ((_thread**)p0) {
                        // Create new thread, only if you have location where to store the handle of it.
                        *((_thread**)p0) = (_thread*)create_tcb((void (*)(void*))p1, (void*)p2, (uint64*)p3);
                        if (*((_thread**)p0)) {
                            Scheduler::get_instance().put_tcb(*((TCB**)p0));
                            k_current_context->a0 = SUCCESS_SYSCALL;
                        }
                    }
                    break;

                case THREAD_EXIT_CODE:
                    if (current_tcb != &main_tcb) {
                        current_tcb->status = TCBStatus::TERMINATING;
                        k_current_context->a0 = SUCCESS_SYSCALL;
                        dispatch();
                    }
                    break;

                case THREAD_DISPATCH_CODE:
                    // Since dispatch system call returns void, we won't change the a0 register value, it really doesn't matter what it has.
                    dispatch();
                    break;

                case THREAD_JOIN_CODE:
                    if ((TCB*)p0 && ((TCB*)p0)->join_sem) {
                        // In case we have pointer to the TCB, and if it has its semaphore, then perform wait on that semaphore.
                        ((TCB*)p0)->join_sem->wait();
                    }
                    break;

                case SEM_OPEN_CODE:
                    if ((_sem**)p0) {
                        // Create semaphore only if you have location to which to save the handle of it.
                        *(_sem**)p0 = (_sem*)Sem::create_sem((int)p1);
                        if (*(_sem**)p0) {
                            k_current_context->a0 = SUCCESS_SYSCALL;
                        }
                    }
                    break;

                case SEM_CLOSE_CODE:
                    if ((Sem*)p0) {
                        temp_val = ((Sem*)p0)->close();
                        if (Sem::free_sem((Sem*)p0) == MemoryAllocator::MEM_SUCCESS) {
                            k_current_context->a0 = temp_val;
                        }
                    }
                    break;

                case SEM_WAIT_CODE:
                    if ((Sem*)p0) {
                        k_current_context->a0 = ((Sem*)p0)->wait();
                    }
                    break;

                case SEM_SIGNAL_CODE:
                    if ((Sem*)p0) {
                        k_current_context->a0 = ((Sem*)p0)->signal();
                    }
                    break;

                case TIME_SLEEP_CODE:
                    if (p0 > 0) {
                        // Sleep the current thread, but only if number of ticks to sleep for are greater than 0, and switch to different thread.
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
        else if (scause_val == SCAUSE_ILLEGAL_INSTRUCTION || scause_val == SCAUSE_LOAD_ACCESS_FAULT || scause_val == SCAUSE_STORE_AMO_ACCESS_FAULT){
            // Empty everything that we have in putc buffer to the console. Because we want to use it.
            flush_putc_buffer();

            // Write specific error message through buffer for putc, based on the scause code. Indeces are: (2 - 2) / 2 = 0; (5 - 2) / 2 = 1; (7 - 2) / 2 = 2;
            const char* messages[3] = { 
                "KERNEL PANIC! ILLEGAL INSTRUCTION AT: ",
                "KERNEL PANIC! ILLEGAL READ OPERATION AT: ",
                "KERNEL PANIC! ILLEGAL WRITE OPERATION AT: "
            };

            int message_idx = (scause_val - 2) / 2;
            for (int i = 0; messages[message_idx][i] != '\0'; i++) {
                putc_buffer.put(messages[message_idx][i]);
            }

            // Write instructino address where the error has occured.
            uint64 weight = Utils::get_decimal_weight(sepc_val);
            while (weight) {
                // Perform integer division on "number" with "weight", take remainder when dividing by 10, and divide "weight" later with 10, so that we take digits from left to right.
                putc_buffer.put('0' + (sepc_val / weight) % 10);
                weight = weight / 10;
            }

            // Append new line, and flush the buffer.
            putc_buffer.put('\n');
            flush_putc_buffer();
            
            // Now be stuck, forever, the user has to restart the machine.
            while(true);
        }
    }

    extern "C" void k_handle_timer(uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6, uint64 a7) {
        // In SIP register, write to the 2nd bit SSIP (SuperVisor Software Interrupt Pending) 0, with that we say that we handled the software interrupt.
        __asm__ volatile("csrc sip, 0x02");

        // Check if there is a thread that needs to be woken up.
        sleep_queue.timer_tick();

        // Increment the timer tick as well, in case the time slice of the current thread has expired, then try switching to another thread.
        if (++timer_ticks >= current_tcb->time_slice) {
            dispatch();
        }
    }

    extern "C" void k_handle_console(uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6, uint64 a7) {
        // Grab the interrupt entry number.
        int irq = plic_claim();

        // To 9-th bit SEIP (SuperVisor External Interupt Pending) write 0, in SIP registry, with that we say that we did handle the hardware interrupt.
        __asm__ volatile("csrc sip, %0" : : "r" (0x200));

        // Tell the PLIC (platform level interrupt controller), that the interrupt has been handled, so that it won't interrupt us again for it.
        plic_complete(irq);

        // Handle the console input, since we were interrupted.
        fill_getc_buffer();
    }


    void prepare_user_mode() {
        // Read sstatus.
        uint64 volatile sstatus_val;
        __asm__ volatile ("csrr %[sts], sstatus" : [sts] "=r" (sstatus_val));

        // Set the 8-th bit to 0, which represents previous mode (now it's user mode), with sret we go back to the previous mode.
        // We set the 8-th bit to 0, by creating a ...00010000000 mask and inverting it to be ...11101111111.
        __asm__ volatile("csrw sstatus, %[sts]" : : [sts] "r" (sstatus_val & ~(1 << 8)));
    }
}