#include "../h/console_lib.hpp"
#include "../h/syscall_cpp.hpp"
#include "../h/k_sem.hpp"
#include "../h/utils.hpp"

static Kernel::Sem console_sem;

static void console_lock() {
    static bool initialized = false;
    if (!initialized) {
        console_sem.initialize(1);
        initialized = true;
    }
    sem_wait((sem_t)&console_sem);
}

static void console_unlock() {
    sem_signal((sem_t)&console_sem);
}


void print_string(const char* message, char end) {
    console_lock();
    for(int i = 0; message[i] != '\0'; i++) {
        Console::putc(message[i]);
    }
    Console::putc(end);
    console_unlock();
}

void print_uint64(uint64 number, char end) {
    uint64 tens = get_decimal_weight(number);

    console_lock();
    while(tens) {
        // Perform integer division on "number" with "tens", and take remainder of it when dividing it with 10, so that we can take the digits from left to right.
        // Also divide the tens by 10, so that we move on to the next digit.
        Console::putc('0' + (number / tens) % 10);
        tens = tens / 10;
    }
    Console::putc(end);
    console_unlock();
}

void print_horizontal_line(int length, char end) {
    console_lock();
    for(int i = 0; i < length; i++) {
        Console::putc('-');
    }
    Console::putc(end);
    console_unlock();
}

char* get_string(int max_length) {
    char* str = new char[max_length + 1];
    str[max_length] = '\0';

    console_lock();
    for(int i = 0; i < max_length && str; i++) {
        str[i] = Console::getc();
        if((int)str[i] == DELETE_KEY) {
            if(i > 0) {
                // "\b \b" will delete the last entered character.
                Console::putc('\b');
                Console::putc(' ');
                Console::putc('\b');
            }
            i = ((i >= 2) ? (i - 2) : -1);
        }
        else if((int)str[i] == ENTER_KEY) {
            str[i] = '\0';
            Console::putc('\n');
            break;
        }
        else {
            Console::putc(str[i]);
        }
    }
    console_unlock();

    return str;
}
