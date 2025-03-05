#include "../h/syscall_c.hpp"
#include "../h/k_syscall_codes.hpp"
#include "../h/utils.hpp"

static uint64 k_system_call(uint64 a0 = 0, uint64 a1 = 0, uint64 a2 = 0, uint64 a3 = 0, uint64 a4 = 0, uint64 a5 = 0, uint64 a6 = 0, uint64 a7 = 0) {
    __asm__ volatile("ecall");
    return a0;
}


void* mem_alloc(size_t size) {
    return (void*)k_system_call(Kernel::MEM_ALLOC_CODE, to_blocks(size));
}

int mem_free(void* address) {
    return (int)k_system_call(Kernel::MEM_FREE_CODE, (uint64)address);
}


int thread_create(thread_t* handle, void(*start_routine)(void*), void* arg) {
    int result_code = Kernel::FAILED_SYSCALL;

    if(handle && start_routine) {
        uint64* stack_space = (uint64*)mem_alloc(DEFAULT_STACK_SIZE);
        if(stack_space) {
            // Nit cemo kreirati, samo ukoliko smo uspesno napravili stek, i ako handle i start_routine pokazuju na nesto.
            result_code = (int)k_system_call(Kernel::CREATE_THREAD_CODE, (uint64)handle, (uint64)start_routine, (uint64)arg, (uint64)&stack_space[DEFAULT_STACK_SIZE / sizeof(uint64)]);
        }
    }

    return result_code;
}

int thread_exit() {
    return (int)k_system_call(Kernel::THREAD_EXIT_CODE);
}

void thread_dispatch() {
    k_system_call(Kernel::THREAD_DISPATCH_CODE);
}

void thread_join(thread_t handle) {
    if(handle) {
        // Radi join, samo ukoliko zaista handle pokazuje na nesto.
        k_system_call(Kernel::THREAD_JOIN_CODE, (uint64)handle);
    }
}


int sem_open(sem_t* handle, unsigned init) {
    if(handle) {
        // Kreiraj semafor, samo ako imas gde da smestis rucku.
        return (int)k_system_call(Kernel::SEM_OPEN_CODE, (uint64)handle, (uint64)init);
    }
    return Kernel::FAILED_SYSCALL;
}

int sem_close(sem_t handle) {
    if(handle) {
        // Zatvori semafor, samo ukoliko handle pokazuje na nesto.
        return (int)k_system_call(Kernel::SEM_CLOSE_CODE, (uint64)handle);
    }
    return Kernel::FAILED_SYSCALL;
}

int sem_wait(sem_t id) {
    if(id) {
        // Uradi operaciju wait nad semaforom, samo ako handle zaista pokazuje na nesto.
        return (int)k_system_call(Kernel::SEM_WAIT_CODE, (uint64)id);
    }
    return Kernel::FAILED_SYSCALL;
}

int sem_signal(sem_t id) {
    if(id) {
        // Uradi operaciju signal nad semaforom, samo ako handle zaista pokazuje na nesto.
        return (int)k_system_call(Kernel::SEM_SIGNAL_CODE, (uint64)id);
    }
    return Kernel::FAILED_SYSCALL;
}


int time_sleep(time_t ticks) {
    if(ticks > 0) {
        // Radi spavanje, samo ako bi trajalo minimum jedan vremenski takt.
        return (int)k_system_call(Kernel::TIME_SLEEP_CODE, ticks);
    }
    return Kernel::FAILED_SYSCALL;
}


void putc(char c) {
    k_system_call(Kernel::PUT_C_CODE, (uint64)c);
}

char getc() {
    return (char)k_system_call(Kernel::GET_C_CODE);
}


int set_user_mode() {
    return (int)k_system_call(Kernel::USER_MODE_CODE);
}
