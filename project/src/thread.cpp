#include "../h/syscall_cpp.hpp"

Thread::Thread(void (*body)(void*), void* arg) {
    this->myHandle = nullptr;
    this->body = body;
    this->arg = arg;
}

Thread::Thread() {
    this->myHandle = nullptr;
    this->body = [](void* t) { ((Thread*)t)->run(); };
    this->arg = this;
}

int Thread::start() {
    if(!this->myHandle) {
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
    this->join();
}
