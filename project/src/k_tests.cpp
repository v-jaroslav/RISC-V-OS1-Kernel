#include "../h/k_tests.hpp"
#include "../h/syscall_cpp.hpp"
#include "../h/console_lib.hpp"

static void print_array(uint64* arr = nullptr, size_t length = 0) {
    for(size_t i = 0; i < length; i++) {
        print_uint64((uint64)&arr[i], ':');
        print_uint64(arr[i]);
    }
    print_string();
}

void KernelTests::memory_test() {
    // For every array of 8 elements, where each element is 8 bytes, we need 1 single block, if the block is 64B, we allocate three blocks.
    uint64* arr1 = (uint64*)mem_alloc(sizeof(uint64) * 8);
    uint64* arr2 = (uint64*)mem_alloc(sizeof(uint64) * 8);
    uint64* arr3 = (uint64*)mem_alloc(sizeof(uint64) * 8);

    for(int i = 0; i < 8; i++) {
        arr1[i] = 100 + i;
        arr2[i] = 200 + i;
        arr3[i] = 300 + i;
    }

    print_array(arr1, 8);
    print_array(arr2, 8);
    print_array(arr3, 8);
    print_horizontal_line(35);


    // Deallocate first two arrays, so basically first two blocks. First three numbers of boths arrays will contain fields of FreeBlocks structure, and they will be merged.
    mem_free(arr2);
    mem_free(arr1);

    // For this array, 3 blocks are necessary if the block is 64B, that way it wont be allocated on previously freed space, but after it.
    uint64* arr4 = (uint64*)mem_alloc(sizeof(uint64) * 17);
    for(int i = 0; i < 17; i++) {
        arr4[i] = 400 + i;
    }

    print_array(arr1, 8);
    print_array(arr2, 8);
    print_array(arr3, 8);
    print_array(arr4, 17);
    print_horizontal_line(35);


    // For this array, 2 blocks are necessary if the size of the block is 64B. It will be allocated in place of first two freed arrays.
    uint64* arr5 = (uint64*)mem_alloc(sizeof(uint64) * 16);
    for(int i = 0; i < 16; i++) {
        arr5[i] = 500 + i;
    }

    print_array(arr1, 8);
    print_array(arr2, 8);
    print_array(arr3, 8);
    print_array(arr4, 17);
    print_array(arr5, 16);
    print_horizontal_line(35);


    // Deallocate third array (1 block), fourth array (3 blocks), fifth array (2 blocks).
    mem_free(arr3);
    mem_free(arr4);
    mem_free(arr5);

    print_array(arr1, 8);
    print_array(arr2, 8);
    print_array(arr3, 8);
    print_array(arr4, 17);
    print_array(arr5, 16);
    print_horizontal_line(35);


    // Tetst overloaded C++ operators.
    uint64* cpp_alloc = new uint64[15];
    for(int i = 0; i < 15; ++i) {
        cpp_alloc[i] = 600 + i;
    }
    print_array(cpp_alloc, 15);
    print_horizontal_line(35);

    delete[] cpp_alloc;
    print_array(cpp_alloc, 15);
    print_horizontal_line(35);
}


namespace {
    void print_loop_standard(void* args) {
        for(int i = 0; i < 2000; i++) {
            print_string((char*)args);
        }
    }

    void print_loop_dispatched(void* args) {
        for(int i = 0; i < 2000; i++) {
            print_string((char*)args);
            Thread::dispatch();
        }
    }

    class CustomThread : public Thread {
        void run() override {
            print_loop_standard((void*)">>> Custom Thread");
        }
    };
}

void KernelTests::threads_test() {
    // Test asynchronous/synchronous preemption.
    Thread s_thr(print_loop_standard, (void*)"+++ Standard Thread");
    Thread d_thr(print_loop_dispatched, (void*)"*** Dispatch Thread");
    CustomThread ct_thr;

    // Start threads.
    s_thr.start();
    d_thr.start();
    ct_thr.start();

    // Test multiple waitings.
    s_thr.join();
    s_thr.join();

    d_thr.join();
    d_thr.join();

    ct_thr.join();
    ct_thr.join();
}


static void synched_printing(void* args) {
    Semaphore* mutex = (Semaphore*)args;

    // Lock the critical section.
    mutex->wait();

    // Some critical section...
    print_string("START: ", ' ');
    for(int i = 0; i < 10; i++) {
        print_uint64(i, ' ');
        thread_dispatch();
    }
    print_string(" :END");

    // Unlock the critical section.
    mutex->signal();
}

void KernelTests::semaphore_test() {
    // Semaphore for mutual exclusion.
    Semaphore mutex(1);

    // Create 5 threads and start them.
    Thread thread_array[5] = {Thread(synched_printing, &mutex),
                              Thread(synched_printing, &mutex),
                              Thread(synched_printing, &mutex),
                              Thread(synched_printing, &mutex),
                              Thread(synched_printing, &mutex)};

    for(int i = 0; i < 5; i++) {
        thread_array[i].start();
    }

    // Wait for all threads to finish.
    for(int i = 0; i < 5; i++) {
        thread_array[i].join();
    }
}


namespace {
    struct SleepParams {
        const char* msg_1;
        const char* msg_2;
        size_t sleep_time;
    };

    void good_night(void* args) {
        SleepParams* sp = (SleepParams*)args;
        print_string(sp->msg_1);
        time_sleep(sp->sleep_time);
        print_string(sp->msg_2);
    }
}

void KernelTests::time_sleep_test() {
    SleepParams a_params = {"THREAD A: ZZzzZzzZZzzZzZZzZz...", "THREAD A: GOOD MORNING!", 25 };
    Thread a_thr(good_night, &a_params);

    SleepParams b_params = {"THREAD B ZZzzZzzZZzzZzZZzZz...", "THREAD B: GOOD MORNING!", 10 };
    Thread b_thr(good_night, &b_params);

    SleepParams c_params = {"THREAD C ZZzzZzzZZzzZzZZzZz...", "THREAD C: GOOD MORNING!", 10 };
    Thread c_thr(good_night, &c_params);

    a_thr.start();
    b_thr.start();
    c_thr.start();

    print_string("KERNEL: ZZzzZzzZZzzZzZZzZz...");
    time_sleep(15);
    print_string("KERNEL: GOOD MORNING!");

    a_thr.join();
    b_thr.join();
    c_thr.join();
}


static void good_bye(void* args) {
    print_string("BEFORE THREAD EXIT");

    // If thread_exit works, the print below shouldn't execute.
    thread_exit();

    print_string("AFTER THREAD EXIT");
}

void KernelTests::thread_exit_test() {
    Thread exit_thr(good_bye, nullptr);
    exit_thr.start();
    exit_thr.join();
}


namespace {
    class PingPong : public PeriodicThread {
    private:
        bool volatile ping;

    protected:
        virtual void periodicActivation() override {
            if (this->ping) {
                print_string("PING");
            }
            else {
                print_string("PONG");
            }
            this->ping = !this->ping;
        }

    public:
        PingPong() : PeriodicThread(1) {
            this->ping = true;
        }
    };
}

void KernelTests::periodic_thread_test() {
    PingPong p;
    p.start();

    getc();
    p.terminate();
    getc();
}


namespace {
    struct IOTestParams {
        Semaphore* io_mutex;
        const char* message;
    };

    void thread_io(void* args) {
        IOTestParams* params = (IOTestParams*)args;

        params->io_mutex->wait();
        print_string(params->message, ' ');
        char* str = get_string();
        print_string(params->message, ' ');
        print_string(str);
        params->io_mutex->signal();

        mem_free(str);
    }
}

void KernelTests::console_io_test() {
    Semaphore console_sem(1);

    IOTestParams a_params = { &console_sem, "Thread A: " };
    Thread a_thr(thread_io, (void*)&a_params);

    IOTestParams b_params = { &console_sem, "Thread B: " };
    Thread b_thr(thread_io, (void*)&b_params);

    IOTestParams c_params = { &console_sem, "Thread C: " };
    Thread c_thr(thread_io, (void*)&c_params);

    a_thr.start();
    b_thr.start();
    c_thr.start();

    a_thr.join();
    b_thr.join();
    c_thr.join();
}


void KernelTests::run_tests() {
    memory_test();

    threads_test();
    thread_exit_test();
    semaphore_test();
    time_sleep_test();
    periodic_thread_test();

    console_io_test();
}
