#include "syscall_cpp.hpp"

void* operator new(size_t bytes) {
    return mem_alloc(bytes);
}

void* operator new[](size_t bytes) {
    return mem_alloc(bytes);
}

void operator delete(void* address) noexcept {
    mem_free(address);
}

void operator delete[](void* address) noexcept {
    mem_free(address);
}
