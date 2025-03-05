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
        // Ako lista nije prazna, nov element treba da pokazuje na glavu, a glava na nov element, nov element postaje glava.
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
        // Ako lista nije prazna, nov element treba da pokazuje na rep, a rep na nov element, nov element postaje rep.
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
        // Pomeri glavu na sledeci element.
        this->head = this->head->next;

        if(!this->head) {
            // Ako je to bio jedini element, postavi rep da pokazuje na nista takodje.
            this->tail = nullptr;
        }
        else {
            // Ako nije bio poslednji element, glava liste ne treba da ima prethodni element.
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
        // Pomeri tail nazad.
        this->tail = this->tail->prev;

        if(!this->tail) {
            // Ako je to bio jedini element, head postavi na nullptr.
            this->head = nullptr;
        }
        else {
            // Ako nije bio poslednji element, rep liste ne treba da ima sledeci element.
            this->tail->next = nullptr;
        }
    }

    List<T>::unlink(t);
    return t;
}
