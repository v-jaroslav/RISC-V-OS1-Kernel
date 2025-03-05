# RISC-V-OS1-Kernel
This repository contains my kernel that I have built as part of the "Operating Systems 1" course at the University of Belgrade at the School of Electrical Engineering.

This kernel has following important characteristics:
- It is supposed to run on RISC-V CPU, specifically RV64IMA architecture.
- It provides ABI that is used by C API, and C++ API that is implemented by using C API (it has layered architecture).
- The kernel supports asynchronous preemption. Meaning it can change the current running thread with another thread uppon interrupt.
- The threads share the CPU across the time, with timed interrupts, by utilizing Round Robin scheduling algorithm.
- Every thread has two stacks, one for user level privilege, and another for kernel operations.
- The kernel does all of this with a single CPU core.


This kernel supports the following C API:
 
| System Call Code |       Signature                                                                                                                            | Explanation                                                                                                                                                                                      |
|------------------|--------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0x01             |    void* mem_alloc (size_t size);                                                                                                          | Allocate at least `size` bytes of memory, rounded up to and alligned with blocks of size `MEM_BLOCK_SIZE`, returns pointer to allocated memory, or null on failure.                              |
| 0x02             |    int mem_free (void*);                                                                                                                   | Frees the memory that was previously allocated with mem_alloc (the argument must be pointer returned from mem_alloc), returns 0 if operation was successful, otherwise negative value.           |
| 0x11             |    class _thread; <br> typedef _thread* thread_t; <br> <br> int thread_create ( thread_t* handle, void(*start_routine)(void*), void* arg); | Start a new thread on |

This kernel suppports the following C++ API:
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


# How to run it?
If you would like to run this kernel and try it out, you must have two things installed on your machine, `git` and `docker`.

First you must clone this repository to your machine with `git clone https://github.com/v-jaroslav/RISC-V-OS1-Kernel`

Second, make sure you are located in the directory of the cloned repository, so just execute `cd RISC-V-OS1-Kernel`

Third, build the docker image that will contain all the tools necessary in order to run the kernel, you can do this with the following command `docker build -f .\main.dockerfile --tag risc-v-kernel .`

Fourth, start the docker container based on the previously built image, in a way that you can interact with its console `docker run -it risc-v-kernel`

Fifth, if you want to ever stop the kernel (by stopping QEMU), just press `CTRL + A` and immediatelly after that press `X`

The docker image you have built, based on that `dockerfile`, contains the necessary cross compilers to compile the code for RISC-V machine.

And it also contains QEMU (open source machine emulator / virtualizer), through which you can run this kernel.

By default, the tests that I have written will execute, and also the public tests that were used to test this kernel at home while developing it.

If you want to change that, you have to provide your own `void userMain()` function, preferrably in `project/src/main.cpp`.

And you will have to comment the one out (or change its signature name) located in: `project/tests/userMain.cpp`

Keep in mind, that this function runs in user level privilege however. To access kernel level privilege, you can do so only indirectly through kernel's API.
