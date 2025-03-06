#include "k_trap_handlers.hpp"
#include "k_tests.hpp"
#include "k_memory.hpp"
#include "k_scheduler.hpp"
#include "syscall_c.hpp"
#include "syscall_cpp.hpp"
#include "k_utils.hpp"


// NOTE: Set this to 0, if you don't want to run the kernel tests that are written by me.
#define RUN_KERNEL_TESTS 1


extern void userMain();

// NOTE: To run your custom userMain() function, simply uncomment this one and just write your code inside of it. It will run instead of userMain() in public tests, as that one is set to be weak symbol.
// void userMain() {
//     Console::print_string("Hello World!");
// }


static void start_userMain() {
    // Create user thread (switch it to user privilege mode as well), start it, and wait for it to finish.
    Thread userMain_thread([](void* args) { set_user_mode(); userMain(); }, nullptr);
    userMain_thread.start();
    userMain_thread.join();
}


// The interrupt table of the kernel is defined externally, so here is the declaration of it, its name shall not be name mangled in C++ way.
extern "C" void k_intr_table();


void main() {
    using namespace Kernel;

    // Set the address of the interupt table, and enable vector interupt mode (so that we can have multiple entries inside of it, we do that by performing binary OR operation with 1 and the address).
    __asm__ volatile ("csrw stvec, %0" : : "r" ((uint64)k_intr_table | 1));

    // Create kernel stack for the main thread for the sake of completeness, and set the SP to point to the &sys_stack[last_index + 1], due to the nature of how RISC V stack behaves.
    main_tcb.sys_stack = (uint64*)MemoryAllocator::get_instance().alloc(Utils::to_blocks(DEFAULT_STACK_SIZE));
    main_tcb.context.sys_sp = (uint64)&main_tcb.sys_stack[DEFAULT_STACK_SIZE / sizeof(uint64)];

    // Initialize the semaphores for console buffers. At the start we can exeucte putc IO_BUFFER_SIZE times since it is empty.
    // And we can execute getc 0 times as its empty, nothing is there to take.
    putc_sem.initialize(IO_BUFFER_SIZE);
    getc_sem.initialize(0);

    // Initialize the Scheduler, which also creates internal putc thread, that flushes the console.
    Scheduler::get_instance().initialize();

    // Set the bit with index 1 (SSIE SuperVisor Software Interrupt Enable) and with index 9 (SEIE Supervisor External Interrupt Enable).
    // This way we will enable software/external interrupts in supervisor mode (basically meaning they aren't masked out).
    __asm__ volatile ("csrw sie, %0" : : "r" (0x202));

    // In sstatus registry, set the bit with index 1 (SIE, SuperVisor Interrupt Enable), so that we can enable all interrupts.
    __asm__ volatile ("csrs sstatus, 0x02");

#if RUN_KERNEL_TESTS == 1
    Kernel::Tests::run_tests();
#endif

    // At the end start the user's program.
    start_userMain();
}
