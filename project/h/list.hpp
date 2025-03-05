#pragma once

// For type T we expect it to have fields next and prev, because that way we can optimize our space complexity.
template<class T>
class List 
{
protected:
    T* head;
    T* tail;

    static void unlink(T* t);
    void set_first(T* t);

public:
    List();

    void initialize();

    void add_first(T* t);
    void add_last(T* t);

    T* take_first();
    T* take_last();

    inline T* peek_first() 
    {
        return this->head;
    }

    inline T* peek_last() 
    {
        return this->tail;
    }

    inline bool is_empty() 
    {
        return (this->head == nullptr);
    }
};

template<class T>
List<T>::List() {
    this->initialize();
}

template<class T>
void List<T>::initialize() {
    this->head = nullptr;
    this->tail = nullptr;
}

template<class T>
void List<T>::unlink(T* t) {
    if(t) {
        t->next = nullptr;
        t->prev = nullptr;
    }
}

template<class T>
void List<T>::set_first(T* t) {
    if(t && this->is_empty()) {
        List<T>::unlink(t);
        this->head = t;
        this->tail = t;
    }
}

template<class T>
void List<T>::add_first(T* t) {
    if(this->is_empty()) {
        this->set_first(t);
        return;
    }

    if(t) {
        // In case list is not empty, the new element has to point to head, and head back to the new element, the new element becomes the new head.
        t->prev = nullptr;
        t->next = this->head;
        this->head->prev = t;
        this->head = t;
    }
}

template<class T>
void List<T>::add_last(T* t) {
    if(this->is_empty()) {
        this->set_first(t);
        return;
    }

    if(t) {
        // If the list is not empty, new elmeent has to point back to the tail, the tail has to point forward to the new element, the new element becomes the tail.
        t->next = nullptr;
        t->prev = this->tail;
        this->tail->next = t;
        this->tail = t;
    }
}

template<class T>
T* List<T>::take_first() {
    T* t = this->head;

    if(!this->is_empty()) {
        // Advance the head to the next element.
        this->head = this->head->next;

        if(!this->head) {
            // If that was the only element, then let the tail also point to nullptr.
            this->tail = nullptr;
        }
        else {
            // If that wasn't the last element, then the head of the list shouldn't have the previous element, as head is always first element.
            this->head->prev = nullptr;
        }
    }

    List<T>::unlink(t);
    return t;
}

template<class T>
T* List<T>::take_last() {
    T* t = this->tail;

    if(!this->is_empty()) {
        // Push tail backwards.
        this->tail = this->tail->prev;

        if(!this->tail) {
            // If it was the last element, let the head point to the null as well.
            this->head = nullptr;
        }
        else {
            // In case it wasn't the last element, we know that tail shouldn't have the next element.
            this->tail->next = nullptr;
        }
    }

    List<T>::unlink(t);
    return t;
}
