#pragma once

template<typename T, int size>
class Queue {
private:
    int head = 0;
    int tail = 0;
    int count = 0;

    T buffer[size];

public:
    T get();
    void put(T t);

    int get_length();
    int get_capacity();

    void clear();
};

template<typename T, int size>
int Queue<T, size>::get_length() {
    // Vraca se broj elemenata u red-u.
    return this->count;
}

template<typename T, int size>
int Queue<T, size>::get_capacity() {
    // Vraca se kapacitet, sablonski parametar.
    return size;
}

template<typename T, int size>
T Queue<T, size>::get() {
    if(this->count > 0) {
        // Po principu kruznog bafera se uzima element.
        T t = this->buffer[this->head];
        this->head = (this->head + 1) % size;
        this->count--;
        return t;
    }
    return T();
}

template<typename T, int size>
void Queue<T, size>::put(T t) {
    if(this->count < size) {
        // Po principu kruznog bafera se dodaje element.
        this->buffer[this->tail] = t;
        this->tail = (this->tail + 1) % size;
        this->count++;
    }
}

template<typename T, int size>
void Queue<T, size>::clear() {
    this->head = 0;
    this->tail = 0;
    this->count = 0;
}
