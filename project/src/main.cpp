#include "../h/k_trap_handlers.hpp"
#include "../h/k_tests.hpp"
#include "../h/k_memory.hpp"
#include "../h/k_scheduler.hpp"
#include "../h/syscall_c.hpp"
#include "../h/syscall_cpp.hpp"
#include "../h/console_lib.hpp"
#include "../h/utils.hpp"

using namespace Kernel;

extern void userMain();

// Example on how to add your own userMain(), however you must comment already existing definition in tests/userMain.cpp
// void userMain() {
//     print_string("Hello World!");
// }

static void start_userMain() {
    // Create user thread, start it, and wait for it to finish.
    Thread userMain_thread([](void* args) { set_user_mode(); userMain(); }, nullptr);
    userMain_thread.start();
    userMain_thread.join();
}


extern "C" void k_intr_table();

void main() {
    // Set the address of interupt table, and enable vector interupt mode (or by one).
    __asm__ volatile ("csrw stvec, %0" : : "r" ((uint64)k_intr_table | 1));

    // Create kernel stack for the main thread for the sake of completeness.
    main_tcb.sys_stack = (uint64*)MemoryAllocator::get_instance().alloc(to_blocks(DEFAULT_STACK_SIZE));
    main_tcb.context.sys_sp = (uint64)&main_tcb.sys_stack[DEFAULT_STACK_SIZE / sizeof(uint64)];

    // Initialize semaphores for the console buffers. At the start we can exeucte putc IO_BUFFER_SIZE times since it is empty.
    // And we can execute getc 0 times as its empty, nothing is there to take.
    putc_sem.initialize(IO_BUFFER_SIZE);
    getc_sem.initialize(0);

    // Initialize the Scheduler, which also creates internal putc thread.
    Scheduler::get_instance().initialize();

    // Set the bit with index 1 (SSIE) and with index 9 (SEIE), such that we enable software and hardware interrupts respectivelly.
    __asm__ volatile ("csrw sie, %0" : : "r" (0x202));

    // In sstatus registry, set the bit with index 1 (SIE), so that we can enable external interrupts.
    __asm__ volatile ("csrs sstatus, 0x02");

    KernelTests::run_tests();
    start_userMain();
}
