#include "syscall_cpp.hpp"


Thread::Thread(void (*body)(void*), void* arg) {
    // The thread is not started yet, we only remember on what function we want to start it, and what argument to pass it.
    this->myHandle = nullptr;
    this->body = body;
    this->arg = arg;
}

Thread::Thread() {
    this->myHandle = nullptr;

    // In case the user didn't pass the body* function pointer, and argument for it.
    // Then we assume that, he is creating a class that is inheriting from the Thread, and thus its run method will be called!
    this->body = [](void* t) { ((Thread*)t)->run(); };
    this->arg = this;
}

int Thread::start() {
    if (!this->myHandle) {
        // Only if thread hasn't been started (in which case the handle is null), we will start it!
        return thread_create(&this->myHandle, this->body, this->arg);
    }

    return THREAD_ALREADY_STARTED;
}

void Thread::join() {
    thread_join(this->myHandle);
}

void Thread::dispatch() {
    thread_dispatch();
}

int Thread::sleep(time_t n_ticks) {
    return time_sleep(n_ticks);
}

Thread::~Thread() {
    // Wait for thread to finish running!
    this->join();
}
