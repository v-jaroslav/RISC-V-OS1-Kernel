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
    // Za svaki niz od 8 elementa od po 8 bajtova neophodan je 1 ceo blok ako je velicina bloka 64B, alociramo tri bloka.
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


    // Dealociramo prva dva niza, prva dva bloka, prva tri broja oba niza ce sadrzati polja FreeBlocks-a, spojice se.
    mem_free(arr2);
    mem_free(arr1);

    // Za ovaj niz, neophodno je 3 bloka ako je blok 64B, time se on nece alocirati na prethodno oslobodjenom prostoru, vec iza poslednje alokacije.
    uint64* arr4 = (uint64*)mem_alloc(sizeof(uint64) * 17);
    for(int i = 0; i < 17; i++) {
        arr4[i] = 400 + i;
    }

    print_array(arr1, 8);
    print_array(arr2, 8);
    print_array(arr3, 8);
    print_array(arr4, 17);
    print_horizontal_line(35);


    // Za ovaj niz, neophodno je 2 bloka ako je velicina bloka 64B. Alocirace se na mesto prva dva oslobodjena niza.
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


    // Dealociramo treci niz jedan blok, cetvrti niz tri bloka, i peti niz dva bloka.
    mem_free(arr3);
    mem_free(arr4);
    mem_free(arr5);

    print_array(arr1, 8);
    print_array(arr2, 8);
    print_array(arr3, 8);
    print_array(arr4, 17);
    print_array(arr5, 16);
    print_horizontal_line(35);


    // Testiranje preklopljenih C++ operatora.
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
    // Testiranje niti asinhrona/sinhrona promena konteksta.
    Thread s_thr(print_loop_standard, (void*)"+++ Standard Thread");
    Thread d_thr(print_loop_dispatched, (void*)"*** Dispatch Thread");
    CustomThread ct_thr;

    // Pokreni niti.
    s_thr.start();
    d_thr.start();
    ct_thr.start();

    // Testiranje visestrukog cekanja.
    s_thr.join();
    s_thr.join();

    d_thr.join();
    d_thr.join();

    ct_thr.join();
    ct_thr.join();
}


static void synched_printing(void* args) {
    Semaphore* mutex = (Semaphore*)args;

    // "Zakljucaj" kriticnu sekciju.
    mutex->wait();

    // Neka kriticna sekcija.
    print_string("START: ", ' ');
    for(int i = 0; i < 10; i++) {
        print_uint64(i, ' ');
        thread_dispatch();
    }
    print_string(" :END");

    // "Otkljucaj" kriticnu sekciju.
    mutex->signal();
}

void KernelTests::semaphore_test() {
    // Semafor za medjusobno iskljucenje.
    Semaphore mutex(1);

    // Napravi 5 niti i pokreni ih.
    Thread thread_array[5] = {Thread(synched_printing, &mutex),
                              Thread(synched_printing, &mutex),
                              Thread(synched_printing, &mutex),
                              Thread(synched_printing, &mutex),
                              Thread(synched_printing, &mutex)};

    for(int i = 0; i < 5; i++) {
        thread_array[i].start();
    }

    // Sacekaj da se sve niti zavrse.
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

    // Ako radi thread_exit, print ispod ne treba da se izvrsi.
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
    // Pokreni test za memoriju.
    memory_test();

    // Pokreni testove za niti.
    threads_test();
    thread_exit_test();
    semaphore_test();
    time_sleep_test();
    periodic_thread_test();

    // Pokreni test konzole.
    console_io_test();
}
