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
    // Return the count of elements in queue.
    return this->count;
}

template<typename T, int size>
int Queue<T, size>::get_capacity() {
    // Return the capacity of the queue, which is basically the template argument.
    return size;
}

template<typename T, int size>
T Queue<T, size>::get() {
    if(this->count > 0) {
        // Based on circular buffer we are taking elements from it.
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
        // Based on circular buffer we are adding element.
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
