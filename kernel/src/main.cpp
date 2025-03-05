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

// void userMain() {
//     print_string("Hello World!");
// }

static void start_userMain() {
    // Kreiraj korisnicku nit, i sacekaj da se zavrsi.
    Thread userMain_thread([](void* args) { set_user_mode(); userMain(); }, nullptr);
    userMain_thread.start();
    userMain_thread.join();
}


extern "C" void k_intr_table();

void main() {
    // Postavi adresu prekidne rutine, i omoguci vektorski rezim prekida.
    __asm__ volatile ("csrw stvec, %0" : : "r" ((uint64)k_intr_table | 1));

    // Kreiraj kernel stek za glavnu nit, radi kompletnosti.
    main_tcb.sys_stack = (uint64*)MemoryAllocator::get_instance().alloc(to_blocks(DEFAULT_STACK_SIZE));
    main_tcb.context.sys_sp = (uint64)&main_tcb.sys_stack[DEFAULT_STACK_SIZE / sizeof(uint64)];

    // Inicijalizuj semafore za bafere konzole. Na pocetku mozemo IO_BUFFER_SIZE izvrsiti puta putc, a 0 puta getc.
    putc_sem.initialize(IO_BUFFER_SIZE);
    getc_sem.initialize(0);

    // Inicijalizuj Scheduler, napravi putc internu nit.
    Scheduler::get_instance().initialize();

    // Postavi 1. bit SSIE i 9. bit SEIE na 1, da se omoguce softverski / hardverski prekidi respektivno.
    __asm__ volatile ("csrw sie, %0" : : "r" (0x202));

    // U sstatus, postavi 1. bit SIE na 1, da se omoguce spoljasnji prekidi.
    __asm__ volatile ("csrs sstatus, 0x02");

    KernelTests::run_tests();
    start_userMain();
}
