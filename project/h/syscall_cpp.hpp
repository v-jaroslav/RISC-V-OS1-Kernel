#pragma once

#include "hw.h"
#include "syscall_c.hpp"

void* operator new(size_t bytes);
void* operator new[](size_t bytes);

void operator delete(void* address) noexcept;
void operator delete[](void* address) noexcept;


// This is what is returned in case the Thread is already started, it is constexpr, so it has internal linkage by default (like static), so it is okay to be here.
constexpr int THREAD_ALREADY_STARTED = -1;

class Thread {
public:
    Thread(void (*body)(void*), void* arg);
    virtual ~Thread();

    int start();
    void join();

    static void dispatch();
    static int sleep(time_t);

protected:
    Thread();
    virtual void run(){}

public:
    thread_t myHandle;
    void (*body)(void*);
    void* arg;
};


class Semaphore {
public:
    Semaphore(unsigned init = 1);
    virtual ~Semaphore();

    int wait();
    int signal();

private:
    sem_t myHandle;
};


class PeriodicThread : public Thread {
public:
    void terminate();

protected:
    PeriodicThread(time_t period);
    
    // This method is supposed to be overriden.
    virtual void periodicActivation() { }

private:
    time_t volatile period;
};


class Console {
public:
    static char getc();
    static void putc(char);
    
    // Extended C++ API, this was added for my own purposes, outside of the project specification.
    static void print_string(const char* message="", char end='\n');
    static void print_uint64(uint64 number=0, char end='\n');
    static char* get_string(int max_length=255);
};
