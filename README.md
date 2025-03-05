# RISC-V-OS1-Kernel
This repository contains my kernel that I have built as part of the "Operating Systems 1" course at the University of Belgrade at the School of Electrical Engineering.

This kernel has following important characteristics:
- It is supposed to run on RISC-V CPU, specifically RV64IMA architecture.
- It provides ABI that is used by C API, and C++ API that is implemented by using C API (it has layered architecture).
- The kernel supports asynchronous preemption. Meaning it can change the current running thread with another thread uppon interrupt.
- The threads share the CPU across the time, with timed interrupts, by utilizing Round Robin scheduling algorithm.
- Every thread has two stacks, one for user level privilege, and another for kernel operations.
- It supports standard input / output through UART protocol.
- It suuports semaphores, a primitive for synchronization, and many other things which you can checkout in the table below.
- The kernel does all of this with a single CPU core.


## This kernel supports the following C API:
 
| System Call Code | Signature                                                                                                                             | Explanation                                                                                                                                                                                                                                  |
|------------------|---------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0x01             | void* mem_alloc(size_t size);                                                                                                         | Allocate at least `size` bytes of memory, rounded up to and alligned with blocks of size `MEM_BLOCK_SIZE`, returns pointer to allocated memory, or null on failure.                                                                          |
| 0x02             | int mem_free(void*);                                                                                                                  | Frees the memory that was previously allocated with mem_alloc (the argument must be pointer returned from mem_alloc), returns 0 if operation was successful, otherwise negative value.                                                       |
| 0x11             | class _thread; <br> typedef _thread* thread_t; <br> <br> int thread_create(thread_t* handle, void(*start_routine)(void*), void* arg); | Start a new thread on `start_routine` function, which will be called with `arg` as its argument. If this succeeds, in `handle` parameter, the handle of the thread will be written, and 0 is returned, otherwise negative value is returned. |
| 0x12             | int thread_exit();                                                                                                                    | Shuts down the current running thread, in case of failure, negative value is returned.                                                                                                                                                       |
| 0x13             | void thread_dispatch();                                                                                                               | Potentially takes away the CPU of the currently running thread and gives it to another thread (potentially to the currently running thread again).                                                                                           |
| 0x14             | void thread_join(thread_t handle);                                                                                                    | Suspend currently running thread, until the `handle` thread finishes.                                                                                                                                                                        |
| 0x21             | class _sem; <br> typedef _sem* sem_t; <br> <br> int sem_open (sem_t* handle, unsigned init);                                          | Create semaphore with initial value `init`. In case of success, the handle of the semaphore is written to the parameter `handle` and 0 is returned, otherwise negative value is returned.                                                    |
| 0x22             | int sem_close(sem_t handle);                                                                                                          | Free the semaphore with specific handle. All the threads that are still waiting on this semaphore get resumed, however their `sem_wait()` returns an error.                                                                                  |
| 0x23             | int sem_wait(sem_t id);                                                                                                               | Execute `wait` operation on a specific semaphore. In case of success, 0 is returned, otherwise negative value is returned.                                                                                                                   |
| 0x24             | int sem_signal(sem_t id);                                                                                                             | Execute `signal` operation on a specific semaphore. In case of success, 0 is returned, otherwise negative value is returned.                                                                                                                 |
| 0x31             | typedef unsigned long time_t; <br> int time_sleep(time_t);                                                                            | Suspend the currently running thread for specific number of internal time ticks. On success 0 is returned, otherwise a negativev value is returned.                                                                                          |
| 0x41             | const int EOF = -1; <br> char getc();                                                                                                 | Read one character from input character buffer for the console, in case the buffer is empty, suspend the currently running thread until some character shows up. The character that was read is returned, on error `EOF` is returned.        |
| 0x42             | void putc(char);                                                                                                                      | Print specific character to the console                                                                                                                                                                                                      |


## This kernel suppports the following C++ API:
```
void* ::operator new (size_t);
void ::operator delete (void*);

class Thread {
public:
    Thread (void (*body)(void*), void* arg);
    virtual ~Thread ();

    int start ();
    void join();

    static void dispatch ();
    static int sleep (time_t);

protected:
    Thread ();
    virtual void run () {}

private:
    thread_t myHandle;
    void (*body)(void*); void* arg;
};

class Semaphore {
public:
    Semaphore (unsigned init = 1);
    virtual ~Semaphore ();

    int wait ();
    int signal ();

private:
    sem_t myHandle;
};

class PeriodicThread : public Thread {
public:
    void terminate ();

protected:
    PeriodicThread (time_t period);
    virtual void periodicActivation () {}

private:
    time_t period;
};

class Console {
public:
    static char getc ();
    static void putc (char);
};
```


## How to run it?
If you would like to run this kernel and try it out, you must have two things installed on your machine, `git` and `docker`.

1. Clone this repository to your machine with `git clone https://github.com/v-jaroslav/RISC-V-OS1-Kernel`

2. Make sure you are located in the directory of the cloned repository, execute `cd RISC-V-OS1-Kernel`

3. Build the docker image based on the `dockerfile`, with the command `docker build -f .\main.dockerfile --tag risc-v-kernel .`

4. Start the docker container based on the previously built image, such that you can interact with its console `docker run -it risc-v-kernel`

5. If you want to ever stop the kernel (by stopping QEMU), just press `CTRL + A` and immediatelly after that press `X`

The docker image you have built, based on that `dockerfile`, contains the necessary cross compilers to compile the code for RISC-V machine. 
It also contains QEMU (open source machine emulator / virtualizer), through which you can run this kernel.

By default, the tests that I have written will execute, and also the public tests (given by faculty) that were used to test this kernel at home while developing it.

If you want to change that, you have to provide your own `void userMain()` function, preferrably in `project/src/main.cpp`. 

And you will have to comment out already given definition for such function (or change its name) at `project/tests/userMain.cpp`

Keep in mind, that this function runs in user level privilege. To access kernel level privilege, you can do so only indirectly through kernel's API.

In `project/src/main.cpp` you can also comment the `KernelTests::run_tests();` line to disable my own tests, it is located in `void main()` function.
