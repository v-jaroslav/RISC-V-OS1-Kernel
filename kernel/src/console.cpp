#include "../h/syscall_cpp.hpp"

char Console::getc() {
    return ::getc();
}

void Console::putc(char c) {
    return ::putc(c);
}
