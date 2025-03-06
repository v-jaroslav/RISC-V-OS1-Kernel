#include "syscall_cpp.hpp"
#include "syscall_cpp.hpp"
#include "k_utils.hpp"
#include "k_sem.hpp"


// This is a semaphore used as mutex to lock/unlock the console. Since it is static, it has internal linkage, so its visible only in this translation unit.
static Kernel::Sem console_sem;

static void console_lock() {
    // If semaphore hasn't been initialized, initialize it.
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


char Console::getc() {
    return ::getc();
}

void Console::putc(char c) {
    ::putc(c);
}


// These are constexpr constants, and therefore they are internal to this translation unit, they have internal linkage, so its fine that they are here.
constexpr int DELETE_KEY = 127;
constexpr int ENTER_KEY  = 13;

// Extended C++ API.
void Console::print_string(const char* message, char end) {
    // Lock the console as we are doing complex output. We don't want threads to be racing each other to do the complex output.
    console_lock();
    for (int i = 0; message[i] != '\0'; i++) {
        Console::putc(message[i]);
    }
    Console::putc(end);
    console_unlock();
}

void Console::print_uint64(uint64 number, char end) {
    uint64 weight = Kernel::Utils::get_decimal_weight(number);
    console_lock();

    while (weight) {
        // Perform integer division on "number" with "weight", and take remainder of it when dividing it with 10, so that we can take the digits from left to right.
        // We add result of that to '0' so we get the digit in char type. Also divide the weight by 10, so that we move on to the next digit. 
        Console::putc('0' + (number / weight) % 10);
        weight = weight / 10;
    }

    Console::putc(end);
    console_unlock();
}

char* Console::get_string(int max_length) {
    // Create a null terminated string on the heap!
    char* str = new char[max_length + 1];
    str[max_length] = '\0';

    // Lock the console, so that in case one thread is doing a complex input, other threads can't take away its input.
    console_lock();

    for (int i = 0; i < max_length && str; ++i) {
        str[i] = Console::getc();

        if ((int)str[i] == DELETE_KEY) {
            if (i > 0) {
                // "\b \b" will delete the last entered character from the console.
                Console::putc('\b');
                Console::putc(' ');
                Console::putc('\b');
            }

            // If we entered a delete key at i-th position, that means we want to ignore previous (i - 1)th char.
            // In reality we want to ignore both of them, so if possible, subtract i by two, otherwise let it be -1, as in next iteration it will be incremented to 0.
            i = ((i >= 2) ? (i - 2) : -1);
        }
        else if ((int)str[i] == ENTER_KEY) {
            // If the user has inputted enter key, null terminate the string then and there, and move to the new line, and get out.
            str[i] = '\0';
            Console::putc('\n');
            break;
        }
        else {
            // Echo back what the user has entered.
            Console::putc(str[i]);
        }
    }

    // After the complex input has been completed, unlock the console and return the entered string.
    console_unlock();
    return str;
}
